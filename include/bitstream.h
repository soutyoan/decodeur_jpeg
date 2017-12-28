#ifndef __BITSTREAM_H__
#define __BITSTREAM_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct bitstream;



/*Affichage pour débogguer le code
0 : n'affiche rien
1 : affichage simplifié de l'exécution
2 : affichage détaillé de l'exécution
*/
#define LOG_BIT 0

/*Fonctions à coder */
extern struct bitstream *create_bitstream(const char *filename);

extern void close_bitstream(struct bitstream *stream);

extern uint8_t read_bitstream(struct bitstream *stream,
                              uint8_t nb_bits,
                              uint32_t *dest,
                              bool discard_byte_stuffing);

extern bool end_of_bitstream(struct bitstream *stream);


extern void skip_bitstream_until(struct bitstream *stream, uint8_t byte);




/* Fonctions auxiliaires */
void affiche_nb_bit(uint32_t byte, uint8_t nb_bits);
void octet_suivant(struct bitstream *stream);

#endif
