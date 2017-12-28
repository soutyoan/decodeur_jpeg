#ifndef __DECODE_JPEG_H__
#define __DECODE_JPEG_H__
/*
    Le module decode_JPEG s'occupe de récupérer les MCUS dans la structure de
    l'image et de les traiter un par un.

    Apres l'appel de decode_image on a tout les MCUs decodes en RGB.
*/

#include "decodage_MCU.h"
#include "types.h"

#define LOG_INIT 0 // passer a 0 si on ne veut pas les logs et 1 sinon
#define LOG_DECODE 0

extern struct image_jpeg *prepare_lecture_MCUs(struct jpeg_desc *jdesc);
extern struct image_jpeg *decode_image_jpeg(struct image_jpeg *image, struct jpeg_desc *jdesc, struct bitstream *stream);

extern void free_image_jpeg(struct image_jpeg *image);

void free_composante_RGB(struct image_jpeg *image, bloc_spatial *blocs_composante);
void free_composante_YCbCr(struct image_jpeg *image, bloc_spatial *blocs_composante, enum component composante);
void affiche_facteurs_echantillonnage(struct image_jpeg *image);
void verifie_echantillonage(struct image_jpeg *image);

#endif
