#ifndef __TYPES_H__
#define __TYPES_H__

#include "huffman.h"
#include "jpeg_reader.h"
#include "bitstream.h"

#include <stdio.h>
#include <stdlib.h>



// un peu de francais ne fait pas de mal (les blocs font toujours 64 elements
// mais on prefere gerer manuellement l'affectation et la liberation des blocs)
typedef int16_t *bloc_frequentiel;
typedef uint8_t *bloc_spatial;

//structure d'un MCU : contient un tableau de blocs spatiaux RGB
struct MCU {
    // On stocke les blocs de chaque composante
    // Dans le cas du niveau de gris, la composante rouge est utilisee
    bloc_spatial *blocs_R;
    bloc_spatial *blocs_G;
    bloc_spatial *blocs_B;
};

//image jpeg dont le champs MCUs est une liste des MCUS qui composent l'image
struct image_jpeg {
    // dimensions en MCU
    uint16_t mcu_height;
    uint16_t mcu_width;
    struct MCU **mcus;

    // dimensions en pixels
    uint16_t pixel_height;
    uint16_t pixels_width;

    // identifiant des composantes
    uint8_t *id_composantes;

    // facteurs d'echatillonages
    uint8_t sampling_vertical_Y; // Ceux-ci nous donnent le nombre de blocs par MCU
    uint8_t sampling_horizontal_Y;

    uint8_t sampling_vertical_Cb;
    uint8_t sampling_horizontal_Cb;

    uint8_t sampling_vertical_Cr;
    uint8_t sampling_horizontal_Cr;

    // Attributs pour le decodage
    uint32_t numero_dernier_bloc_decode; // celui ci sert juste pour le debug
    uint16_t derniere_valeur_DC_Y;
    uint16_t derniere_valeur_DC_Cb;
    uint16_t derniere_valeur_DC_Cr;
    // 1 pour niveau de gris, 3 pour YCbCr
    uint8_t nb_composante;
};

#endif
