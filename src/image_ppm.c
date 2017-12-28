#include "../include/image_ppm.h"

void create_image_ppm(struct image_jpeg *image, const char *nom_fichier_entree) {
    FILE *fichier_sortie = NULL;
    char *nom_sortie = nom_fichier_sortie(nom_fichier_entree, image -> nb_composante);
    fichier_sortie = fopen(nom_sortie, "wb");
    // verifie_allocation("Erreur lors de la creation de l'image", fichier_sortie);

    char *format_sortie = "P5";
    if(image -> nb_composante == 3) format_sortie = "P6";
    fprintf(fichier_sortie, "%s\n%u %u\n%d\n", format_sortie, image->pixels_width, image->pixel_height, 255);

    // on a de la chance les deux fonctions d'écriture de pixels prennent les meme arguments
    // on peut utiliser un pointeur pour savoir laquelle utiliser
    void (*write_pixel)(struct image_jpeg *image, FILE *image_gris, uint16_t x, uint16_t y);
    write_pixel = write_grey_pixel;
    if(image -> nb_composante == 3) write_pixel = write_color_pixel;

    for (uint16_t y = 0; y < image -> pixel_height; y++) {
        for (uint16_t x = 0; x < image -> pixels_width; x++) {
            (*write_pixel)(image, fichier_sortie, x, y);
        }
    }

    fclose(fichier_sortie);

    printf("%s créée avec succès\n\n", nom_sortie);
    free(nom_sortie);
}

// Place le pixel gris aux coordonnees (x, y) dans le fichier de sortie
void write_grey_pixel(struct image_jpeg *image, FILE *image_gris, uint16_t x, uint16_t y) {
    uint32_t numero_mcu = image -> mcu_width * (y/8) + x / 8;
    uint16_t numero_pixel = (y % 8)*8 + x%8;
    uint8_t niveau_gris = image -> mcus[numero_mcu] -> blocs_R[0][numero_pixel];
    fwrite(&niveau_gris, sizeof(uint8_t), 1, image_gris);
}

// Fait la meme chose mais avec un pixel de couleur
void write_color_pixel(struct image_jpeg *image, FILE *image_couleur, uint16_t x, uint16_t y) {
    uint16_t num_mcu = (image -> mcu_width)*(y/(8*image -> sampling_vertical_Y)) + x/(8*image -> sampling_horizontal_Y);
    uint16_t num_bloc = (x%(8*image -> sampling_horizontal_Y))/8 + image -> sampling_horizontal_Y*((y%(8*image -> sampling_vertical_Y))/8);
    uint16_t num_pix = (y % 8)*8 + x%8;

    uint8_t red_color = image -> mcus[num_mcu] -> blocs_R[num_bloc][num_pix];
    uint8_t green_color = image -> mcus[num_mcu] -> blocs_G[num_bloc][num_pix];
    uint8_t blue_color = image -> mcus[num_mcu] -> blocs_B[num_bloc][num_pix];

    fwrite(&red_color, sizeof(uint8_t), 1, image_couleur);
    fwrite(&green_color, sizeof(uint8_t), 1, image_couleur);
    fwrite(&blue_color, sizeof(uint8_t), 1, image_couleur);
}

// Fonction qui calcule le nom de sortie du fichier
char* nom_fichier_sortie(const char *filename, uint8_t nb_composante) {
    int index_point = 0;
    char* nom_sortie = malloc(40*sizeof(char));
    strcpy(nom_sortie, filename);

    while(filename[index_point] != '.')
        index_point ++;
    nom_sortie[index_point + 1] = 'p';
    nom_sortie[index_point + 2] = 'p';
    if(nb_composante == 1) nom_sortie[index_point + 2] = 'g';
    nom_sortie[index_point + 3] = 'm';
    nom_sortie[index_point + 4] = '\0';

    return nom_sortie;
}
