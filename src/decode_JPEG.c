#include "../include/decode_JPEG.h"

// -----------------------------------------------------------------------------
// Fonction qui prepare la lecture et le traitement des MCUs (disposition dans
// l'image, etc)
struct image_jpeg *prepare_lecture_MCUs(struct jpeg_desc *jdesc) {
    struct image_jpeg *image = malloc(sizeof(struct image_jpeg));
    if(image == NULL) {
        fprintf(stderr, "Erreur lors de l'allocation de la matrice de MCUs\n");
        exit(1);
    }

    image -> nb_composante = get_nb_components(jdesc);

    image -> id_composantes = malloc(sizeof(uint8_t) * image -> nb_composante);
    for(uint8_t i = 0; i < image -> nb_composante; i ++)
        image -> id_composantes[i] = get_frame_component_id(jdesc, i);

    // On stocke les facteurs d'echantillonnage
    image -> sampling_horizontal_Y = get_frame_component_sampling_factor(jdesc, DIR_H, 0);
    image -> sampling_vertical_Y = get_frame_component_sampling_factor(jdesc, DIR_V, 0);
    image -> sampling_horizontal_Cb = get_frame_component_sampling_factor(jdesc, DIR_H, 1);
    image -> sampling_vertical_Cb = get_frame_component_sampling_factor(jdesc, DIR_V, 1);
    image -> sampling_horizontal_Cr = get_frame_component_sampling_factor(jdesc, DIR_H, 2);
    image -> sampling_vertical_Cr = get_frame_component_sampling_factor(jdesc, DIR_V, 2);

    verifie_echantillonage(image);

    // On calcule nos dimensions en MCU et on alloue la place necessaire
    image -> mcu_height = (get_image_size(jdesc, DIR_V)-1) / (8*image -> sampling_vertical_Y) + 1;
    image -> mcu_width = (get_image_size(jdesc, DIR_H)-1) / (8*image -> sampling_horizontal_Y) + 1;
    image -> mcus = malloc(sizeof(struct MCU*) * (image -> mcu_height) * (image -> mcu_width));

    image -> pixel_height = get_image_size(jdesc, DIR_V);
    image -> pixels_width = get_image_size(jdesc, DIR_H);

    // Quelques variables pour le decodage et le numero du dernier bloc sert pour le debug
    image -> numero_dernier_bloc_decode = 0;
    image -> derniere_valeur_DC_Y = 0;
    image -> derniere_valeur_DC_Cb = 0;
    image -> derniere_valeur_DC_Cr = 0;

    printf("Dimensions : %ux%u\n", image->pixels_width, image -> pixel_height);
    if(image -> nb_composante == 3) affiche_facteurs_echantillonnage(image);
    else printf("Niveaux de gris\n");

    if(LOG_INIT > 0) printf("Hauteur en MCU: %u Largeur en MCU: %u\n", image -> mcu_height, image -> mcu_width);

    // C'est pret on peut y aller
    return image;
}

// -----------------------------------------------------------------------------
// Fonction qui lit puis decode tout les MCUs et remplit la matrice (enfin le tableau mais dans nos coeurs une image reste en 2D) de MCUs
struct image_jpeg *decode_image_jpeg(struct image_jpeg *image, struct jpeg_desc *jdesc, struct bitstream *stream) {
    if(LOG_DECODE > 0) printf("Début du décodage des MCUs\n");
    // On cree le tableau de MCUs
    uint32_t nombre_MCU = image -> mcu_width * image -> mcu_height;
    if(LOG_DECODE > 0) printf("Nombres de MCUs %u\n", nombre_MCU);

    // Et c'est parti
    for(uint32_t i = 0; i < nombre_MCU; i ++)
        image -> mcus[i] = decode_prochain_MCU(image, jdesc, stream);

    if(LOG_DECODE > 0) printf("Fin du décodage des MCUs\n");

    // C'est decode, on envoie tout ca a l'affichage
    return image;
}

// -----------------------------------------------------------------------------
// Fonction qui libere l'image
void free_image_jpeg(struct image_jpeg *image) {
    uint16_t nb_MCUs = image -> mcu_width * image -> mcu_height;
    // on libere les MCUs un par un
    for(uint16_t i = 0; i < nb_MCUs; i ++) {
        struct MCU *mcu_courant = image -> mcus[i];
        free_composante_RGB(image, mcu_courant -> blocs_R);
        // les autres composantes si on est en couleur
        if(image -> nb_composante == 3) {
            free_composante_RGB(image, mcu_courant -> blocs_G);
            free_composante_RGB(image, mcu_courant -> blocs_B);
        }
        free(mcu_courant);
    }

    free(image -> mcus);
    free(image -> id_composantes);
    free(image);
}

/*******************************************************************************
Fonctions utilitaires
*******************************************************************************/

// Fonction qui libère une composante RGB (toutes les composantes font la meme taille)
void free_composante_RGB(struct image_jpeg *image, bloc_spatial *blocs_composante) {
    uint8_t nb_blocs_par_composante = image -> sampling_horizontal_Y * image -> sampling_vertical_Y;
    for(uint8_t i = 0; i < nb_blocs_par_composante; i ++)
        free(blocs_composante[i]);
    free(blocs_composante);
}

// Fonction qui libère une composante Y Cb ou Cr
void free_composante_YCbCr(struct image_jpeg *image, bloc_spatial *blocs_composante, enum component composante) {
    uint8_t nb_blocs_par_composante = image -> sampling_horizontal_Y * image -> sampling_vertical_Y;
    if(composante == COMP_Cb) nb_blocs_par_composante = image -> sampling_horizontal_Cb * image -> sampling_vertical_Cb;
    else if(composante == COMP_Cr) nb_blocs_par_composante = image -> sampling_horizontal_Cr * image -> sampling_vertical_Cr;
    for(uint8_t i = 0; i < nb_blocs_par_composante; i ++)
        free(blocs_composante[i]);
    free(blocs_composante);
}

void affiche_facteurs_echantillonnage(struct image_jpeg *image) {
    printf("Facteurs d'echantillonnage : %ux%u %ux%u %ux%u\n",
        image -> sampling_horizontal_Y,
        image -> sampling_vertical_Y,
        image -> sampling_horizontal_Cb,
        image -> sampling_vertical_Cb,
        image -> sampling_horizontal_Cr,
        image -> sampling_vertical_Cr
    );
}

// Verifie qu'on a bien pas d'echantillonnage en niveau de gris et que la somme des produits des facteurs est bien inférieure a 10 pour la couleur
void verifie_echantillonage(struct image_jpeg *image) {
    uint8_t produit_echantillonage_Y = image -> sampling_vertical_Y * image -> sampling_horizontal_Y;
    if(image -> nb_composante == 1) {
        if(produit_echantillonage_Y != 1) {
            fprintf(stderr, "Les facteurs d'echantillonnage ne valent pas 1 pour une image en niveau de gris (et c'est très bizarre)\n");
            exit(1);
        }
    } else if(produit_echantillonage_Y + image -> sampling_vertical_Cb * image -> sampling_horizontal_Cb + image -> sampling_vertical_Cr * image -> sampling_horizontal_Cr > 10) {
        fprintf(stderr, "Facteurs d'echantillonnage invalides, la somme des produits des facteurs de chaque composante est superieur à 10\n");
        exit(1);
    }
}
