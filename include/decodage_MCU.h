#ifndef __DECODE_MCU_H__
#define __DECODE_MCU_H__
/*
    Le module decodage_MCU s'occupe de toute la partie definie comme 2. dans le
    resume des etapes du sujet.

    On lui demande de decoder le prochain MCU du fichier, il effectue tout le
    pipeline de la decompression jusqu'a la conversion en pixels RGB (ou juste R
    qui joue le role du gris dans une image en niveau de gris).
*/

#include "decode_JPEG.h"
#include "types.h"

#include <math.h>

#define LOG_LECTURE_FREQUENCE 0 // passer a 0 si on ne veut pas les logs
// 1 si on veut juste voir le resultat et 2 si on veut un mode bavard
#define LOG_QUANTIFICATION 0
#define LOG_IDCT 0
#define LOG_RGB 0
#define LOG_ZZ 0

#define TAILLE_BLOC 64

#define PI 3.14159265




// Fonction qui effectue le decodage complet d'un MCU depuis la lecture dans le
// stream jusqu'a la reconversion en RGB
extern struct MCU *decode_prochain_MCU(struct image_jpeg *jpeg_desc, struct jpeg_desc *jdesc, struct bitstream *stream);
bloc_spatial lecture_traitement_bloc(struct jpeg_desc *jdesc, struct bitstream *stream, struct image_jpeg *image, enum component composante, uint8_t i_eme_composante);

/* pipeline de decodage d'un MCU */
// on lit le bloc de frequences brut dans le fichier
extern bloc_frequentiel lire_bloc_frequence(struct jpeg_desc *jdesc, struct bitstream *stream, uint32_t numero_bloc, int16_t prec_DC, uint8_t i_eme_composante);
// on multiplie ce "vecteur" par la table de quantification associee
extern bloc_frequentiel quantification_inverse(struct jpeg_desc *jdesc, bloc_frequentiel frequences_brutes, uint8_t i_eme_composante);
// on remet les frequences dans l'ordre
extern bloc_frequentiel zig_zag_inverse(bloc_frequentiel tab);
// on repasse dans le domaine spatial en YCbCr
extern bloc_spatial iDCT_old(bloc_frequentiel tab);
// on convertit ensuite tout ca en RGB avec eventuellement un coup d'upsampling
extern void conversion_vers_RGB(struct image_jpeg *image, struct MCU *nouveau_MCU, bloc_spatial *tabY, bloc_spatial *tabCb, bloc_spatial *tabCr);
extern bloc_spatial iDCT(bloc_frequentiel tab);
extern float* loeffler(float* stage_4);
extern void Rotation(float x, float y, float k, uint8_t n, float *z, float *t);
extern void butterfly(float x, float y, float *z, float *t);


/* fonction utilitaires */
bloc_spatial *prochain_tableau_composante(struct image_jpeg *image, bloc_spatial *blocs_Y, bloc_spatial *blocs_Cb, bloc_spatial *blocs_Cr, uint8_t identifiant_composante);
uint8_t nb_blocs_par_MCU(struct image_jpeg *image, uint8_t identifiant_composante);
enum component composante_associee(struct image_jpeg *image, uint8_t identifiant_composante);

uint16_t derniere_valeur_DC_decodee(struct image_jpeg *image, enum component composante);
void actualise_derniere_valeur_DC(struct image_jpeg *image, enum component composante, uint16_t derniere_valeur_DC);
int16_t magnitude(uint8_t classe_magnitude, uint32_t code);

void afficher_bloc_spatial(char *message, bloc_spatial bloc_couleurs);
void afficher_bloc_frequentiel(char *message, bloc_frequentiel bloc_frequences);

// Fonction pour verifier la lecture du bitstream, quitte si le nombre de bits lus n'est pas celui attendu
void v_read_bitstream(struct bitstream *stream, uint8_t nb_bits, uint32_t *dest, bool discard_byte_stuffing);
void verifie_allocation(char *message, void *pointeur_a_tester);
void clamp_0_255(float *float_a_clamper);

#endif
