#include "../include/decodage_MCU.h"

// -----------------------------------------------------------------------------
// Fonction qui effectue le decodage complet d'un MCU depuis la lecture dans le
// stream jusqu'a la reconversion en RGB
struct MCU *decode_prochain_MCU(struct image_jpeg *image, struct jpeg_desc *jdesc, struct bitstream *stream) {
    // on calcule la taille memoire necessaire pour stocker une composante du MCU
    size_t taille_memoire_blocs_MCU = sizeof(bloc_spatial) * image -> sampling_horizontal_Y * image -> sampling_vertical_Y;
    // on aloue notre nouveau MCU et ses composantes
    struct MCU *nouveau_MCU = malloc(sizeof(struct MCU));
    nouveau_MCU -> blocs_R = malloc(taille_memoire_blocs_MCU);
    nouveau_MCU -> blocs_G = NULL;
    nouveau_MCU -> blocs_B = NULL;
    verifie_allocation("Erreur lors de l'allocation d'un nouveau MCU", nouveau_MCU);
    verifie_allocation("Erreur lors de l'allocation de la premiere composante", nouveau_MCU -> blocs_R);

    if(image -> nb_composante == 3) {
        nouveau_MCU -> blocs_G = malloc(taille_memoire_blocs_MCU);
        nouveau_MCU -> blocs_B = malloc(taille_memoire_blocs_MCU);
        verifie_allocation("Erreur lors de l'allocation d'une composante d'un MCU", nouveau_MCU -> blocs_G);
        verifie_allocation("Erreur lors de l'allocation d'une composante d'un MCU", nouveau_MCU -> blocs_B);
    }

    // On prepare le decodage du MCU avec l'allocation des blocs spatiaux de
    // sortie juste avant l'upsampling et la conversion RGB
    bloc_spatial *blocs_Y = malloc(taille_memoire_blocs_MCU);
    verifie_allocation("Erreur lors de l'allocation pour la composante Y", blocs_Y);
    bloc_spatial *blocs_Cb = NULL;
    bloc_spatial *blocs_Cr = NULL;

    // Dans le cas de la couleur on traite les deux nouvelles composantes de la meme maniere
    if(image -> nb_composante == 3) {
        // On prevoie plus de place par ce qu'on va mettre les blocs RGB apres l'upsampling dedans
        blocs_Cb = malloc(taille_memoire_blocs_MCU);
        blocs_Cr = malloc(taille_memoire_blocs_MCU);
        verifie_allocation("Erreur lors de l'allocation pour la composante Cb", blocs_Cb);
        verifie_allocation("Erreur lors de l'allocation pour la composante Cr", blocs_Cr);
    }

    // On utilise nos fonctions rajoutees a la derniere minute pour lire les composantes si elles ne sont pas dans le bon ordre
    uint8_t nb_blocs_composante, id_composante_courant;
    bloc_spatial *blocs_composante_courants;
    for(uint8_t i = 0; i < image -> nb_composante; i ++) {
        id_composante_courant = get_scan_component_id(jdesc, i);
        nb_blocs_composante = nb_blocs_par_MCU(image, id_composante_courant);
        blocs_composante_courants = prochain_tableau_composante(image, blocs_Y, blocs_Cb, blocs_Cr, id_composante_courant);
        for(uint8_t j = 0; j < nb_blocs_composante; j ++)
            blocs_composante_courants[j] = lecture_traitement_bloc(jdesc, stream, image, composante_associee(image, id_composante_courant), i);
    }

    // Une fois qu'on a lu notre MCU dans le flux, on va realiser l'upsampling
    // et la conversion de YCbCr vers RGB d'un coup
    conversion_vers_RGB(image, nouveau_MCU, blocs_Y, blocs_Cb, blocs_Cr);

    // Si on est en couleur, on libere nos blocs YCbCr
    if(image -> nb_composante == 3) {
        free_composante_YCbCr(image, blocs_Y, COMP_Y);
        free_composante_YCbCr(image, blocs_Cb, COMP_Cb);
        free_composante_YCbCr(image, blocs_Cr, COMP_Cr);
    } else free(blocs_Y);

    // Voila on a un MCU avec ses composantes RGB tout pret a etre affiche dans une image
    return nouveau_MCU;
}

// Fonction pour le traitement d'un seul bloc, de sa lecture, a la sortie de la
// transformee inverse (on finit avec un bloc d'une composante parmis YCbCr).
bloc_spatial lecture_traitement_bloc(struct jpeg_desc *jdesc, struct bitstream *stream, struct image_jpeg *image, enum component composante, uint8_t i_eme_composante) {
    // On lit d'abord les frequences brutes dans le flux
    bloc_frequentiel bloc_frequentiel_brut = lire_bloc_frequence(jdesc, stream, image -> numero_dernier_bloc_decode, derniere_valeur_DC_decodee(image, composante), i_eme_composante);
    actualise_derniere_valeur_DC(image, composante, bloc_frequentiel_brut[0]);
    // On effectue la quantification inverse
    bloc_frequentiel frequences_requantifiees = quantification_inverse(jdesc, bloc_frequentiel_brut, i_eme_composante);
    // Le zig-zag inverse
    bloc_frequentiel zig_zag_frequences = zig_zag_inverse(frequences_requantifiees);
    // Et on repasse dans le domaine spatial Y, Cb ou Cr
    bloc_spatial pixels_composante = iDCT(zig_zag_frequences);

    free(frequences_requantifiees);
    free(zig_zag_frequences);
    // On a ici un bloc de 64 elements d'une des 3 composantes Y Cb ou Cr lu dans le flux
    return pixels_composante;
}

/*******************************************************************************
Pipeline JPEG
*******************************************************************************/

// -----------------------------------------------------------------------------
// Fonction qui lit un bloc frequentiel du flux de bits
bloc_frequentiel lire_bloc_frequence(struct jpeg_desc *jdesc, struct bitstream *stream, uint32_t numero_bloc, int16_t prec_DC, uint8_t i_eme_composante) {
    // Alocation du bloc frequentiel
    bloc_frequentiel frequences_brutes = malloc(sizeof(int16_t) * TAILLE_BLOC);
    verifie_allocation("Erreur lors de l'allocation d'un bloc de frequence", frequences_brutes);

    // Chargement des tables de huffman (juste une affectation de pointeurs, on
    // le fait a chaque bloc)
    struct huff_table *huff_DC = get_huffman_table(jdesc, DC, get_scan_component_huffman_index(jdesc, DC, i_eme_composante));
    struct huff_table *huff_AC = get_huffman_table(jdesc, AC, get_scan_component_huffman_index(jdesc, AC, i_eme_composante));

    if(LOG_LECTURE_FREQUENCE > 0) printf("\n-- Lecture du bloc frequentiel %u --\n", numero_bloc);

    uint32_t byte = 0;
    // Lecture de la composante continue ***************************************
    uint8_t classe_magnitude_DC = next_huffman_value(huff_DC, stream);
    v_read_bitstream(stream, classe_magnitude_DC, &byte, true);
    frequences_brutes[0] = magnitude(classe_magnitude_DC, byte) + prec_DC;
    if(LOG_LECTURE_FREQUENCE > 1) printf("Composante continue: %u 0x%x\n", byte, byte);

    // Lecture des composantes alternatives ************************************
    uint8_t classe_magnitude, RLE, nb_zero, i = 1;
    int16_t frequence;
    do {
        // Lecture du premier octet RLE
        RLE = next_huffman_value(huff_AC, stream);
        // Si on a un token de 16 composantes nulles on les ajoute et on passe a la prochaine valeure
        if(RLE == 0xf0) {
            for(uint8_t j = 0; j < 16; j ++)
                frequences_brutes[i+j] = 0;
            i += 16;
            if(LOG_LECTURE_FREQUENCE > 1) printf("Bloc de 16 zeros\n");
            continue;
        }
        // on prend les 4 premiers bits pour le nombre de zero avant la prochaine valeur non nulle
        nb_zero = RLE >> 4;
        // et les 4 derniers pour la classe de magnitude
        classe_magnitude = RLE & 0xf;

        if(LOG_LECTURE_FREQUENCE > 1) printf("Nombre zeros: %u Classe magnitude: %u ", nb_zero, classe_magnitude);

        // on peut alors ajouter le nombre de zero necessaire
        for(uint8_t j = 0; j < nb_zero; j++) {
            frequences_brutes[i] = 0;
            i ++;
        }
        // et lire la prochaine valeure non nulle (tant pis si on a une fin de bloc)
        // a penser que lire 0 bits doit bien fonctionner et... ne pas avancer le flux
        v_read_bitstream(stream, classe_magnitude, &byte, true);
        // Calcul de la valeur alternative dans la table de magnitude
        frequence = magnitude(classe_magnitude, byte);

        if(LOG_LECTURE_FREQUENCE > 1) printf("Composante alternative: %hi 0x%x\n", frequence, frequence);

        // on peut ajouter notre frequence fraichement decodee et passer a la suivante
        frequences_brutes[i] = frequence;
        i ++;

    } while(RLE != 0 && i < 64); // 0x00 <=> End Of Block

    // On complete les dernieres frequences par des 0
    for(uint8_t j = i; j < TAILLE_BLOC; j ++) {
        i ++;
        frequences_brutes[j] = 0;
    }

    if(LOG_LECTURE_FREQUENCE > 0) afficher_bloc_frequentiel("Bloc frequentiel brut: ", frequences_brutes);

    // on peut envoyer notre bloc de 64 valeurs frequentielles a la quantification inverse
    return frequences_brutes;
}

// -----------------------------------------------------------------------------
// Fonction qui effecture la quantification inverse
bloc_frequentiel quantification_inverse(struct jpeg_desc *jdesc, bloc_frequentiel frequences_brutes, uint8_t i_eme_composante) {
    uint8_t *table_qantification = get_quantization_table(jdesc, get_frame_component_quant_index(jdesc, i_eme_composante));
    // on multiplie gentillement composante par composante les frequences
    for(uint8_t i = 0; i < TAILLE_BLOC; i ++)
        frequences_brutes[i] *= table_qantification[i];
    if(LOG_QUANTIFICATION > 0) afficher_bloc_frequentiel("Bloc frequentiel apres quantification inverse: ", frequences_brutes);

    // on envoie les frequences requantifiees au zig-zag inverse
    return frequences_brutes;
}


/* zig_zag_inverse prend en arguement un bloc_frequentiel tab (tableau de 64 int16_t) et renvoie le bloc
  reorganise selon l'ordre zig_zag */
bloc_frequentiel zig_zag_inverse(bloc_frequentiel tab){
    bloc_frequentiel zz_bloc = malloc(TAILLE_BLOC*sizeof(int16_t));
    verifie_allocation("Erreur lors de l'allocation du bloc frequentiel pour le zig-zag", zz_bloc);

    uint16_t sum, x = 0, y = 0;
    for(uint16_t i = 0; i < TAILLE_BLOC; i++) {
        zz_bloc[8*y + x] = tab[i];
        sum = x + y;
        if(sum%2 == 0) {
            if(y == 0) x += 1;
            else if(x == 7) y += 1;
            else {
                x += 1;
                y -= 1;
            }
        } else {
            if(y == 7) x += 1;
            else if(x==0) y += 1;
            else {
                y += 1;
                x -= 1;
            }
        }
    }
    if(LOG_ZZ > 0) afficher_bloc_frequentiel("Bloc frequentiel apres zig-zag inverse: ", zz_bloc);
    return zz_bloc;
}

void butterfly(float x, float y, float *z, float *t){
	*z = (x + y)/2;
	*t = (x - y)/2;
}

void Rotation(float x, float y, float k, uint8_t n, float *z, float *t){
	*z = ( x*cos((n*PI)/16) - y*sin((n*PI)/16) )/k;
	*t = ( y*cos((n*PI)/16) + x*sin((n*PI)/16) )/k;
}

/*  fonction qui effectue les transformations decrites dans la transformation de loeffler*/
float* loeffler(float* stage_4){

    // float racine_un_demi = 1/sqrt(2);
    float stage_3[8];
    float stage_2[8];
    float stage_1[8];
    float stage_0[8];
    // float racine_2 = sqrt(2);
    for(uint8_t i=0; i<8 ;i++){
        stage_4[i]=sqrt(8)*stage_4[i];
    }

    // etape 1
    stage_3[0] = stage_4[0];
    stage_3[4] = stage_4[4];
    stage_3[2] = stage_4[2];
    stage_3[6] = stage_4[6];
    butterfly(stage_4[1],stage_4[7],&stage_3[1],&stage_3[7]);
    stage_3[3] = stage_4[3]/sqrt(2);
    stage_3[5] = stage_4[5]/sqrt(2);

    // etape 2

    butterfly(stage_3[0],stage_3[4],&stage_2[0],&stage_2[4]);
    Rotation(stage_3[2],stage_3[6],sqrt(2),6,&stage_2[2],&stage_2[6]);
    butterfly(stage_3[7],stage_3[5],&stage_2[7],&stage_2[5]);
    butterfly(stage_3[1],stage_3[3],&stage_2[1],&stage_2[3]);

    // etape 3
    butterfly(stage_2[0],stage_2[6],&stage_1[0],&stage_1[6]);
    butterfly(stage_2[4],stage_2[2],&stage_1[4],&stage_1[2]);
    Rotation(stage_2[7],stage_2[1],1,3,&stage_1[7],&stage_1[1]);
    Rotation(stage_2[3],stage_2[5],1,1,&stage_1[3],&stage_1[5]);

    // etape 4
    butterfly(stage_1[0],stage_1[1],&stage_0[0],&stage_0[1]);
    butterfly(stage_1[4],stage_1[5],&stage_0[4],&stage_0[5]);
    butterfly(stage_1[2],stage_1[3],&stage_0[2],&stage_0[3]);
    butterfly(stage_1[6],stage_1[7],&stage_0[6],&stage_0[7]);


    float* orig = malloc(8*sizeof(float));
    orig[0] = stage_0[0];
    orig[1] = stage_0[4];
    orig[2] = stage_0[2];
    orig[3] = stage_0[6];
    orig[4] = stage_0[7];
    orig[5] = stage_0[3];
    orig[6] = stage_0[5];
    orig[7] = stage_0[1];

    return orig;
}
/* Fonction qui prend un pointeur vers un tableau de 64 entiers (bloc
frequentiel) et renvoie un tableau de 64 entiers représentant le bloc spatial)*/

bloc_spatial iDCT(bloc_frequentiel tab){
    bloc_spatial tab_spatiale = malloc(TAILLE_BLOC*sizeof(uint8_t));
    float tab_tmp[TAILLE_BLOC];

    for(uint8_t colonne = 0; colonne < 8; colonne++){
        float bloc_colonne[8];
        for(uint8_t ligne = 0; ligne < 8; ligne++){
            bloc_colonne[ligne] = (float)tab[8*ligne + colonne];
        }
        float* bloc_0 = loeffler(bloc_colonne);
        for(uint8_t ligne = 0; ligne < 8; ligne++){
            tab_tmp[8*ligne + colonne] = bloc_0[ligne];
        }
        free(bloc_0);
    }

    for(uint8_t ligne = 0; ligne < 8; ligne++){
        float bloc_ligne[8];
        for(uint8_t colonne = 0; colonne < 8; colonne++){
            bloc_ligne[colonne] = tab_tmp[8*ligne + colonne];
        }
        float* bloc_0_2 = loeffler(bloc_ligne);
        for(uint8_t colonne = 0; colonne < 8; colonne++){
            float valeur_tmp = bloc_0_2[colonne];
            //    valeur_tmp = valeur_tmp*8;
            valeur_tmp += 128;
            if(valeur_tmp < 0){
                valeur_tmp = 0;
            }
            if(valeur_tmp > 255){
                valeur_tmp = 255;
            }
            uint8_t valeur = (uint8_t) valeur_tmp;
            tab_spatiale[8*ligne + colonne] = valeur;
        }
        free(bloc_0_2);
    }


    if(LOG_IDCT > 0) afficher_bloc_spatial("Bloc spatial YCbCr: ", tab_spatiale);

    return tab_spatiale;
}


/* Fonction qui prend un pointeur vers un tableau de 64 entiers (bloc
frequentiel) et renvoie un tableau de 64 entiers représentant le bloc spatial)*/
bloc_spatial iDCT_old(bloc_frequentiel tab){
    int n = 8;
    float Ci,Cj;
    bloc_spatial tab_spatiale = malloc(TAILLE_BLOC*sizeof(uint8_t));
    for (int indice = 0; indice < TAILLE_BLOC; indice++){
        int x = indice%n;
        int y = (indice - x)/n;
        float valeur_tmp = 0;
        for(int i = 0; i < n; i++){
            for (int j = 0; j < n; j++){
                if(i == 0){
                    Ci = 1/sqrt(2);
                }else{
                    Ci = 1;
                }
                if(j == 0){
                    Cj = 1/sqrt(2);
                }else{
                    Cj = 1;
                }
                int indice_frequence = n*j + i;
                int16_t phi = tab[indice_frequence];
                valeur_tmp += Ci*Cj*cos(((2*x + 1)*i*PI)/(2*n))*cos(((2*y + 1)*j*PI)/(2*n))*phi;
            }
        }
        valeur_tmp = valeur_tmp/4;
        valeur_tmp += 128;
        clamp_0_255(&valeur_tmp);
        uint8_t valeur = (uint8_t) valeur_tmp;
        tab_spatiale[indice] = valeur;
    }
    if(LOG_IDCT > 0) afficher_bloc_spatial("Bloc spatial YCbCr: ", tab_spatiale);
    return tab_spatiale;
}

// -----------------------------------------------------------------------------
/* fonction pour transformer les donnes YCbCr en RGB */
void conversion_vers_RGB(struct image_jpeg *image, struct MCU *nouveau_MCU, bloc_spatial *tabY, bloc_spatial *tabCb, bloc_spatial *tabCr){
    // niveau de gris
    if((tabCb == NULL) && (tabCr == NULL)){
        if(LOG_RGB > 0) afficher_bloc_spatial("Bloc spatial RGB: ", tabY[0]);
        nouveau_MCU -> blocs_R[0] = tabY[0];
    // couleur
    } else {
        for(uint8_t num_bloc=0; num_bloc < (image -> sampling_horizontal_Y)*(image -> sampling_vertical_Y); num_bloc++) {
            bloc_spatial tab_R = malloc(TAILLE_BLOC*sizeof(uint8_t));
            bloc_spatial tab_G = malloc(TAILLE_BLOC*sizeof(uint8_t));
            bloc_spatial tab_B = malloc(TAILLE_BLOC*sizeof(uint8_t));
            for(uint8_t j=0; j < TAILLE_BLOC; j++){
                // On calcule les coordonnees dans tout le MCU
                uint8_t abscisse = 8*(num_bloc % image -> sampling_horizontal_Y) + j % 8;
                uint8_t ordonnee = 8*(num_bloc / image -> sampling_horizontal_Y) + j / 8;
                // On les ramene dans un bloc
                abscisse /= image -> sampling_horizontal_Y;
                ordonnee /= image -> sampling_vertical_Y;
                // On peut alors caluler l'indice pour l'upsampling
                uint8_t i = abscisse + 8 * ordonnee;

                float R = tabY[num_bloc][j] - 0.0009267*(tabCb[0][i] - 128) + 1.4016868*(tabCr[0][i] - 128);
                float G = tabY[num_bloc][j] - 0.3436954*(tabCb[0][i] - 128) - 0.7141690*(tabCr[0][i] - 128);
                float B = tabY[num_bloc][j] + 1.7721604*(tabCb[0][i] - 128) + 0.0009902*(tabCr[0][i] - 128);

                clamp_0_255(&R);
                clamp_0_255(&G);
                clamp_0_255(&B);

                tab_R[j] = (uint8_t) R;
                tab_G[j] = (uint8_t) G;
                tab_B[j] = (uint8_t) B;
            }
            nouveau_MCU -> blocs_R[num_bloc] = tab_R;
            nouveau_MCU -> blocs_G[num_bloc] = tab_G;
            nouveau_MCU -> blocs_B[num_bloc] = tab_B;

        }
    }
}





/*******************************************************************************
Fonction utilitaires et de debug
*******************************************************************************/

// Fonction pour rattraper les maladresses qui renvoie le tableau qui va acceuillir la prochaine composante
bloc_spatial *prochain_tableau_composante(struct image_jpeg *image, bloc_spatial *blocs_Y, bloc_spatial *blocs_Cb, bloc_spatial *blocs_Cr, uint8_t identifiant_composante) {
    if(image -> id_composantes[0] == identifiant_composante) return blocs_Y;
    if(image -> id_composantes[1] == identifiant_composante) return blocs_Cb;
    if(image -> id_composantes[2] == identifiant_composante) return blocs_Cr;
    fprintf(stderr, "Identifiant invalide dans le scan JPEG\n");
    exit(1);
}

// La meme chose mais pour le nombre de blocs par MCU de la composante
uint8_t nb_blocs_par_MCU(struct image_jpeg *image, uint8_t identifiant_composante) {
    if(image -> id_composantes[0] == identifiant_composante) return image -> sampling_horizontal_Y * image -> sampling_vertical_Y;
    if(image -> id_composantes[1] == identifiant_composante) return image -> sampling_horizontal_Cb * image -> sampling_vertical_Cb;
    if(image -> id_composantes[2] == identifiant_composante) return image -> sampling_horizontal_Cr * image -> sampling_vertical_Cr;
    fprintf(stderr, "Identifiant invalide dans le scan JPEG\n");
    exit(1);
}

// Encore mais pour la valeur de l'enum qui represente l'identifiant de la composante en entree
enum component composante_associee(struct image_jpeg *image, uint8_t identifiant_composante) {
    if(image -> id_composantes[0] == identifiant_composante) return COMP_Y;
    if(image -> id_composantes[1] == identifiant_composante) return COMP_Cb;
    if(image -> id_composantes[2] == identifiant_composante) return COMP_Cr;
    fprintf(stderr, "Identifiant invalide dans le scan JPEG\n");
    exit(1);
}

// Fonction qui renvoie la derniere valeur DC decodee de la composante courante
uint16_t derniere_valeur_DC_decodee(struct image_jpeg *image, enum component composante) {
    if(composante == COMP_Y)
        return image -> derniere_valeur_DC_Y;
    else if (composante == COMP_Cb)
        return image -> derniere_valeur_DC_Cb;
    else
        return image -> derniere_valeur_DC_Cr;
}

// Fonction qui actualise la derniere valeur DC decodee de la composante courante
void actualise_derniere_valeur_DC(struct image_jpeg *image, enum component composante, uint16_t derniere_valeur_DC) {
    image -> numero_dernier_bloc_decode += 1; // On place le bout pour le debug ici c'est toujours ca de moins a voir en haut
    if(composante == COMP_Y)
        image -> derniere_valeur_DC_Y = derniere_valeur_DC;
    else if (composante == COMP_Cb)
        image -> derniere_valeur_DC_Cb = derniere_valeur_DC;
    else
        image -> derniere_valeur_DC_Cr = derniere_valeur_DC;
}

// Fonction qui renvoie la magnitude d'un element selon sa classe et son code
int16_t magnitude(uint8_t classe_magnitude, uint32_t code) {
    int16_t magnitude_calculee = code;
    // pour les nombres negatifs
    if(code >> (classe_magnitude-1) == 0)
        magnitude_calculee += -(1 << classe_magnitude) + 1;

    return magnitude_calculee;
}

// Fonction pour verifier la lecture du bitstream
void v_read_bitstream(struct bitstream *stream, uint8_t nb_bits, uint32_t *dest, bool discard_byte_stuffing) {
    if(read_bitstream(stream, nb_bits, dest, discard_byte_stuffing) != nb_bits) {
        fprintf(stderr, "Erreur de lecture du bitstream ! (lecture de %u bits)\n", nb_bits);
        exit(1);
    }
}

// Fonction qui affiche le contenu d'un bloc spatial
void afficher_bloc_spatial(char *message, bloc_spatial bloc_couleurs) {
    printf("%s\n", message);
    for(uint8_t i = 0; i < TAILLE_BLOC; i ++) {
        printf("%hhx ", bloc_couleurs[i]);
    }
    printf("\n");
    printf("\n");
}

// Fonction qui affiche le contenu d'un bloc frequentiel
void afficher_bloc_frequentiel(char *message, bloc_frequentiel bloc_frequences) {
    printf("%s\n", message);
    for(uint8_t i = 0; i < TAILLE_BLOC; i ++) {
        printf("%hx ", bloc_frequences[i]);
    }
    printf("\n");
}

// Fonction qui verifie si un pointeur n'est pas NULL (en gros si malloc a marche)
void verifie_allocation(char *message, void *pointeur_a_tester) {
    if(pointeur_a_tester == NULL) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

// Fonction qui replace le float donne entre 0 et 255
void clamp_0_255(float *float_a_clamper) {
    if(*float_a_clamper > 255) *float_a_clamper = 255;
    if(*float_a_clamper < 0) *float_a_clamper = 0;
}
