#include "../include/bitstream.h"

struct bitstream {
    //Pointeur sur la lecture du fichier
    FILE *file;
    //Savoir s'il reste des octets à lire
    bool is_empty;
    //Octet en cours de lecture
    uint8_t current_octet;
    //Indice du bit lu dans l'octet current (numérotation du poids fort au poids faible)
    uint8_t num_bit;
};


// Création d'un flux de bits à lire
struct bitstream *create_bitstream(const char *filename){
    FILE *image = fopen(filename, "rb");
    if(image == NULL) {
        fprintf(stderr, "Impossible de créer le bitstream pour le fichier %s\n", filename);
        return NULL;
    }
    struct bitstream *stream = malloc(sizeof(struct bitstream));
    if (stream == NULL){
        fprintf(stderr, "Flux bitstream non déclaré en mémoire. \n");
        exit(1);
    }
    if (fread(&stream->current_octet, sizeof(uint8_t), 1, image) == 0){
        fclose(image);
        return NULL;
    } else {
        stream->num_bit = 0;
        stream->is_empty = false;
        stream->file = image;
        return stream;
    }
}

void close_bitstream(struct bitstream *stream){
    fclose(stream->file);
    free(stream);
}


//Lecture de nb_bits dans le flux stream et écriture dans le pointeur *dest
uint8_t read_bitstream(struct bitstream *stream, uint8_t nb_bits, uint32_t *dest, bool discard_byte_stuffing){
    if (nb_bits>32){
        fprintf(stderr, "Lecture de 32 bits max par appel de la fonction. \n");
        exit(1);
    }
    uint8_t nb_bits_lus = 0, bit_lu;
    *dest = 0;
    while (nb_bits_lus<nb_bits && !stream->is_empty){
        bit_lu = stream->current_octet >> (7-stream->num_bit) & 1;
        *dest = (*dest<<1) + bit_lu;
        nb_bits_lus+=1;
        stream->num_bit+=1;
        if (stream->num_bit==8){
            if (stream->current_octet == 0xff){
            // Cas où on doit faire un (ou des) byte stuffing
                octet_suivant(stream);
                if (discard_byte_stuffing && stream->current_octet == 0x00){
                    octet_suivant(stream);
                }
            } else {
                octet_suivant(stream);
            }
        }
    }
    return nb_bits_lus;
}

//Retourne true si le flux a été entièrement lu, false sinon
bool end_of_bitstream(struct bitstream *stream){
    return (stream->is_empty);
}

//Permet d'avancer dans le flux jusqu'à avoir un octet égal à byte.
void skip_bitstream_until(struct bitstream *stream, uint8_t byte){
    while (!stream->is_empty > 0 && stream->current_octet != byte){
        octet_suivant(stream);
    }
}

//FONCTIONS AUXILIAIRES

//Affiche les nb_bits premiers bit de l'entier byte
void affiche_nb_bit(uint32_t byte, uint8_t nb_bits){
    printf("Lecture de %u bits dans %x :\n", nb_bits, byte);
    for (uint8_t i = nb_bits; i>0; i--){
        uint8_t bit = byte>>(i-1) & 1;
        printf("%u", bit);
    }
    printf("\n\n");
}



//Supprime l'octet en tête d'un flux et positionne le flux au 1er bit de l'octet suivant
void octet_suivant(struct bitstream *stream){
    if (fread(&stream->current_octet, sizeof(uint8_t), 1, stream->file) == 0){
        stream->is_empty = true;
    }
    stream->num_bit = 0;
  }
