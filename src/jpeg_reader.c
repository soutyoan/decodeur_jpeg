#include "../include/jpeg_reader.h"
#include <stdlib.h>
#include <stdio.h>

struct jpeg_desc{
    // dimensions en pixels
    uint16_t pixels_height;
    uint16_t pixels_width;
    uint8_t nb_huffman_tables_AC ;
    uint8_t nb_huffman_tables_DC ;
    struct huff_table** huffman_table_AC;
    struct huff_table** huffman_table_DC;

    uint8_t nb_quantization_tables;
    uint8_t** quantization_tables;
    // facteurs d'echatillonages
    uint8_t sampling_vertical_Y; // Ceux-ci nous donnent le nombre de blocs par MCU
    uint8_t sampling_horizontal_Y;

    uint8_t sampling_vertical_Cb;
    uint8_t sampling_horizontal_Cb;

    uint8_t sampling_vertical_Cr;
    uint8_t sampling_horizontal_Cr;
// pour reconnaitre l'ordre des blocs dans le SOS
    uint8_t scan_0;
    uint8_t scan_1;
    uint8_t scan_2;

    uint8_t indice_huffman_Y_DC;
    uint8_t indice_huffman_Y_AC;

    uint8_t indice_huffman_Cb_DC;
    uint8_t indice_huffman_Cb_AC;

    uint8_t indice_huffman_Cr_DC;
    uint8_t indice_huffman_Cr_AC;

    uint8_t nb_composantes;
    uint8_t indice_component_Y;
    uint8_t indice_component_Cb;
    uint8_t indice_component_Cr;

    uint8_t indice_component_quant_Y;
    uint8_t indice_component_quant_Cb;
    uint8_t indice_component_quant_Cr;

    struct bitstream* stream;

    char* filename;
};

struct jpeg_desc *read_jpeg(const char *filename){
    struct jpeg_desc* jpeg = malloc(sizeof(struct jpeg_desc));
    jpeg -> filename = (char*)filename;
    jpeg -> nb_huffman_tables_AC = 0;
    jpeg -> nb_huffman_tables_DC = 0;
    jpeg -> huffman_table_AC = malloc(4*sizeof(struct huff_table*));
    jpeg -> huffman_table_DC = malloc(4*sizeof(struct huff_table*));
    jpeg -> quantization_tables = malloc(4*sizeof(uint8_t*));
    for(uint8_t indice = 0; indice < 4; indice++){
        jpeg -> huffman_table_AC[indice] = NULL;
        jpeg -> huffman_table_DC[indice] = NULL;
        jpeg -> quantization_tables[indice] = NULL;
    }
    jpeg -> nb_quantization_tables = 0;
    struct bitstream *stream;
    uint8_t longeur;
    stream = create_bitstream(filename);
    uint32_t bits_tmp;
    uint8_t nb_read = read_bitstream(stream, 16, &bits_tmp,false);
    if(bits_tmp != 0xffd8){
        fprintf(stderr, "Ce n'est pas une image JPEG\n");
        exit(1);
    }
    skip_bitstream_until(stream,  0xdb);
    nb_read = read_bitstream(stream, 8, &bits_tmp,false);
    bool stop = false;
    while(!stop){
        switch (bits_tmp) {
            case 0xc0:
                nb_read = read_bitstream(stream, 24, &bits_tmp,false);
                nb_read = read_bitstream(stream, 16, &bits_tmp,false);
                jpeg -> pixels_height = bits_tmp;
                nb_read = read_bitstream(stream, 16, &bits_tmp,false);
                jpeg -> pixels_width = bits_tmp;
                nb_read = read_bitstream(stream, 8, &bits_tmp,false);
                jpeg -> nb_composantes = bits_tmp;
                if(jpeg -> nb_composantes == 1){
                    nb_read = read_bitstream(stream, 8, &bits_tmp,false);
                    jpeg -> indice_component_Y = bits_tmp;
                    nb_read = read_bitstream(stream, 4, &bits_tmp,false);
                    jpeg -> sampling_horizontal_Y = bits_tmp;
                    nb_read = read_bitstream(stream, 4, &bits_tmp,false);
                    jpeg -> sampling_vertical_Y = bits_tmp;
                    nb_read = read_bitstream(stream, 8, &bits_tmp,false);
                    jpeg -> indice_component_quant_Y = bits_tmp;
                }else{
                    nb_read = read_bitstream(stream, 8, &bits_tmp,false);
                    jpeg -> indice_component_Y = bits_tmp;
                    nb_read = read_bitstream(stream, 4, &bits_tmp,false);
                    jpeg -> sampling_horizontal_Y = bits_tmp;
                    nb_read = read_bitstream(stream, 4, &bits_tmp,false);
                    jpeg -> sampling_vertical_Y = bits_tmp;
                    nb_read = read_bitstream(stream, 8, &bits_tmp,false);
                    jpeg -> indice_component_quant_Y = bits_tmp;

                    nb_read = read_bitstream(stream, 8, &bits_tmp,false);
                    jpeg -> indice_component_Cb = bits_tmp;
                    nb_read = read_bitstream(stream, 4, &bits_tmp,false);
                    jpeg -> sampling_horizontal_Cb = bits_tmp;
                    nb_read = read_bitstream(stream, 4, &bits_tmp,false);
                    jpeg -> sampling_vertical_Cb = bits_tmp;
                    nb_read = read_bitstream(stream, 8, &bits_tmp,false);
                    jpeg -> indice_component_quant_Cb = bits_tmp;

                    nb_read = read_bitstream(stream, 8, &bits_tmp,false);
                    jpeg -> indice_component_Cr = bits_tmp;
                    nb_read = read_bitstream(stream, 4, &bits_tmp,false);
                    jpeg -> sampling_horizontal_Cr = bits_tmp;
                    nb_read = read_bitstream(stream, 4, &bits_tmp,false);
                    jpeg -> sampling_vertical_Cr = bits_tmp;
                    nb_read = read_bitstream(stream, 8, &bits_tmp,false);
                    jpeg -> indice_component_quant_Cr = bits_tmp;
                }
                    skip_bitstream_until(stream, 0xff);
                    nb_read = read_bitstream(stream, 8, &bits_tmp,false);
                    nb_read = read_bitstream(stream, 8, &bits_tmp,false);
                    break;
            case 0xdb:
                nb_read = read_bitstream(stream, 16, &bits_tmp,false);
                longeur = bits_tmp;
                longeur -= 2;
                uint8_t nb_tables = longeur/65;
                jpeg -> nb_quantization_tables += nb_tables;
                for(uint8_t i = 0; i < nb_tables;i++){
                        uint8_t* quant_table = malloc(64*sizeof(uint8_t));
                        nb_read = read_bitstream(stream, 4, &bits_tmp,false);
                        nb_read = read_bitstream(stream, 4, &bits_tmp,false);
                        uint8_t indice = bits_tmp;
                        for(uint8_t j = 0; j < 64 ;j++){
                            nb_read = read_bitstream(stream, 8, &bits_tmp,false);
                            quant_table[j] = bits_tmp;

                        }
                        jpeg -> quantization_tables[indice] = quant_table;
                }
                skip_bitstream_until(stream, 0xff);
                nb_read = read_bitstream(stream, 8, &bits_tmp,false);
                nb_read = read_bitstream(stream, 8, &bits_tmp,false);
                break;

            case 0xc4:
                nb_read = read_bitstream(stream, 16, &bits_tmp,false);
                uint16_t longeur = bits_tmp - 2;
                uint16_t bytes_read = 0;
                while (bytes_read < longeur){
                    nb_read = read_bitstream(stream, 3, &bits_tmp,false);
                    nb_read = read_bitstream(stream, 1, &bits_tmp,false);
                    enum acdc type = bits_tmp;
                    uint16_t i = 0;
                    uint16_t* nb_byte_read = &i ;
                    if(type == DC){
                        jpeg -> nb_huffman_tables_DC += 1;
                        nb_read = read_bitstream(stream, 4, &bits_tmp,false);
                        bytes_read += 1;
                        uint8_t indice = bits_tmp;
                        jpeg -> huffman_table_DC[indice] = load_huffman_table(stream, nb_byte_read);
                        bytes_read += *nb_byte_read;
                    }else{
                        jpeg -> nb_huffman_tables_AC += 1;
                        nb_read = read_bitstream(stream, 4, &bits_tmp,false);
                        bytes_read += 1;
                        uint8_t indice = bits_tmp;
                        *nb_byte_read = 0;
                        jpeg -> huffman_table_AC[indice] = load_huffman_table(stream, nb_byte_read);
                        bytes_read += *nb_byte_read;
                    }
                }
                skip_bitstream_until(stream,  0xff);
                nb_read = read_bitstream(stream, 8, &bits_tmp,false);
                nb_read = read_bitstream(stream, 8, &bits_tmp,false);
                break;

            case 0xda:
                nb_read = read_bitstream(stream, 16, &bits_tmp,false);
                longeur = bits_tmp;
                nb_read = read_bitstream(stream, 8, &bits_tmp,false);
                if(jpeg -> nb_composantes == 1){
                    nb_read = read_bitstream(stream, 8, &bits_tmp,false);
                    jpeg -> scan_0 = bits_tmp;
                    nb_read = read_bitstream(stream, 4, &bits_tmp,false);
                    jpeg -> indice_huffman_Y_DC = bits_tmp;
                    nb_read = read_bitstream(stream, 4, &bits_tmp,false);
                    jpeg -> indice_huffman_Y_AC = bits_tmp;
                }else{
                    for (uint8_t i = 0; i < 3; i++) {
                        uint32_t bits_tmp_2;
                        nb_read = read_bitstream(stream, 8, &bits_tmp_2,false);
                        if(i == 0){
                            jpeg -> scan_0 = bits_tmp_2;
                        }else if(i == 1){
                            jpeg -> scan_1 = bits_tmp_2;
                        }else{
                            jpeg -> scan_2 = bits_tmp_2;
                        }
                        if(bits_tmp_2 == jpeg -> indice_component_Y){
                            nb_read = read_bitstream(stream, 4, &bits_tmp,false);
                            jpeg -> indice_huffman_Y_DC = bits_tmp;
                            nb_read = read_bitstream(stream, 4, &bits_tmp,false);
                            jpeg -> indice_huffman_Y_AC = bits_tmp;
                        }else if(bits_tmp_2 == jpeg -> indice_component_Cb){
                            nb_read = read_bitstream(stream, 4, &bits_tmp,false);
                            jpeg -> indice_huffman_Cb_DC = bits_tmp;
                            nb_read = read_bitstream(stream, 4, &bits_tmp,false);
                            jpeg -> indice_huffman_Cb_AC = bits_tmp;
                        }else{
                            nb_read = read_bitstream(stream, 4, &bits_tmp,false);
                            jpeg -> indice_huffman_Cr_DC = bits_tmp;
                            nb_read = read_bitstream(stream, 4, &bits_tmp,false);
                            jpeg -> indice_huffman_Cr_AC = bits_tmp;
                        }

                    }
                }
                nb_read = read_bitstream(stream, 24, &bits_tmp,false);
                jpeg -> stream = stream;
                stop = true;
                break;
            case 0xe0:
                skip_bitstream_until(stream,  0xff);
                break;
            case 0xfe:
                skip_bitstream_until(stream,  0xff);
                break;
            default:
                fprintf(stderr, "Ce cas n'est pas traite\n");
                exit(1);
                break;
        }
    }
    return jpeg;
}

void close_jpeg(struct jpeg_desc *jpeg) {
    for(uint8_t indice = 0; indice < 4; indice++){
        if(jpeg -> huffman_table_AC[indice] != NULL){
            free_huffman_table(jpeg -> huffman_table_AC[indice]);
        }
        if(jpeg -> huffman_table_DC[indice] != NULL){
            free_huffman_table(jpeg -> huffman_table_DC[indice]);
        }
        if(jpeg -> quantization_tables[indice] != NULL){
            free(jpeg -> quantization_tables[indice]);
        }
    }
    free(jpeg -> huffman_table_AC);
    free(jpeg -> huffman_table_DC);
    free(jpeg -> quantization_tables);
    close_bitstream(jpeg -> stream);
    free(jpeg);
}

char *get_filename(const struct jpeg_desc *jpeg){
    return jpeg -> filename;
}


struct bitstream *get_bitstream(const struct jpeg_desc *jpeg){
    return jpeg -> stream;
}



uint8_t get_nb_quantization_tables(const struct jpeg_desc *jpeg){
    return jpeg -> nb_quantization_tables;
}
uint8_t *get_quantization_table(const struct jpeg_desc *jpeg,uint8_t index){
    return jpeg -> quantization_tables[index];
}



// from DHT
uint8_t get_nb_huffman_tables(const struct jpeg_desc *jpeg, enum acdc acdc){
    if(acdc == AC){
        return jpeg -> nb_huffman_tables_AC;
    }else{
        return jpeg -> nb_huffman_tables_DC;
    }
}


struct huff_table *get_huffman_table(const struct jpeg_desc *jpeg,enum acdc acdc, uint8_t index){
    if(acdc == AC){
        return jpeg -> huffman_table_AC[index];
    }else{
        return jpeg -> huffman_table_DC[index];
    }
}

// from Frame Header SOF0
uint16_t get_image_size(struct jpeg_desc *jpeg, enum direction dir){
    if(dir == DIR_H){
        return jpeg -> pixels_width;
    }else{
        return jpeg -> pixels_height;
    }
}

uint8_t get_nb_components(const struct jpeg_desc *jpeg){
    return jpeg -> nb_composantes;
}

uint8_t get_frame_component_id(const struct jpeg_desc *jpeg, uint8_t frame_comp_index){
    if(frame_comp_index == 0){
        return jpeg -> indice_component_Y;
    }else if(frame_comp_index == 1){
        return jpeg -> indice_component_Cb;
    }else{
        return jpeg -> indice_component_Cr;
    }
}

uint8_t get_frame_component_sampling_factor(const struct jpeg_desc *jpeg,
                                            enum direction dir, uint8_t frame_comp_index){
    if(frame_comp_index == 0){
        if(dir == DIR_H){
            return jpeg -> sampling_horizontal_Y;
        }else{
            return jpeg -> sampling_vertical_Y;
        }
    }else if(frame_comp_index == 1){
        if(dir == DIR_H){
            return jpeg -> sampling_horizontal_Cb;
        }else{
            return jpeg -> sampling_vertical_Cb;
        }
    }else{
        if(dir == DIR_H){
            return jpeg -> sampling_horizontal_Cr;
        }else{
            return jpeg -> sampling_vertical_Cr;
        }
    }

}
uint8_t get_frame_component_quant_index(const struct jpeg_desc *jpeg,
                                               uint8_t frame_comp_index){
    if(frame_comp_index == 0){
       return jpeg -> indice_component_quant_Y;
    }else if(frame_comp_index == 1){
       return jpeg -> indice_component_quant_Cb;
    }else{
       return jpeg -> indice_component_quant_Cr;
    }

}
// from Scan Header SOS
uint8_t get_scan_component_id(const struct jpeg_desc *jpeg,
                                     uint8_t scan_comp_index){
    if(scan_comp_index == 0){
        return jpeg -> scan_0;
    }else if(scan_comp_index == 1){
        return jpeg -> scan_1;
    }else{
        return jpeg -> scan_2;
    }

                                     }
uint8_t get_scan_component_huffman_index(const struct jpeg_desc *jpeg,enum acdc acdc,uint8_t scan_comp_index){
    if(scan_comp_index == 0){
        if(acdc == AC){
            return jpeg -> indice_huffman_Y_AC;
        }else{
            return jpeg -> indice_huffman_Y_DC;
        }
    }else if(scan_comp_index == 1){
        if(acdc == AC){
            return jpeg -> indice_huffman_Cb_AC;
        }else{
            return jpeg -> indice_huffman_Cb_DC;
        }
    }else{
        if(acdc == AC){
            return jpeg -> indice_huffman_Cr_AC;
        }else{
            return jpeg -> indice_huffman_Cr_DC;
        }
    }
}
