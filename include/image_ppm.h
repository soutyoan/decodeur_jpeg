#ifndef _CREATE_IMAGE_H
#define _CREATE_IMAGE_H

#include "decodage_MCU.h"
#include "types.h"
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#define LOG_IMAGE_PGM 0 //0 pour ne rien afficher dans le terminal, 1 pour afficher début et fin, 2 pour tout afficher

extern void create_image_ppm(struct image_jpeg *image, const char *nom_fichier_entree);
void write_grey_pixel(struct image_jpeg *image, FILE *image_gris, uint16_t x, uint16_t y);
void write_color_pixel(struct image_jpeg *image, FILE *image_couleur, uint16_t x, uint16_t y);

char* nom_fichier_sortie(const char *filename, uint8_t nb_composante);

#endif
