# Repertoires du projet

BIN_DIR = bin
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj


# Options de compilation/édition des liens

CC = clang
LD = clang
INC = -I$(INC_DIR)

CFLAGS += $(INC) -Wall -std=c99 -O0 -g -Wextra
LDFLAGS = -lm

# Liste des fichiers objet


OBJ_FILES =  $(OBJ_DIR)/main.o $(OBJ_DIR)/decodage_MCU.o $(OBJ_DIR)/image_ppm.o $(OBJ_DIR)/decode_JPEG.o   $(OBJ_DIR)/jpeg_reader.o $(OBJ_DIR)/bitstream.o $(OBJ_DIR)/huffman.o

# cible par défaut

TARGET = $(BIN_DIR)/jpeg2ppm

all: $(TARGET)

$(TARGET):  $(OBJ_FILES)
	$(LD) $(LDFLAGS) $(OBJ_FILES)  -pg -o $(TARGET)

$(OBJ_DIR)/main.o: $(SRC_DIR)/main.c $(INC_DIR)/jpeg_reader.h $(INC_DIR)/bitstream.h $(INC_DIR)/decodage_MCU.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/main.c -pg -o $(OBJ_DIR)/main.o

$(OBJ_DIR)/decodage_MCU.o: $(SRC_DIR)/decodage_MCU.c $(INC_DIR)/jpeg_reader.h $(INC_DIR)/bitstream.h $(INC_DIR)/huffman.h $(INC_DIR)/decodage_MCU.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/decodage_MCU.c -pg -o $(OBJ_DIR)/decodage_MCU.o

$(OBJ_DIR)/image_ppm.o: $(SRC_DIR)/image_ppm.c $(INC_DIR)/image_ppm.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/image_ppm.c -pg -o $(OBJ_DIR)/image_ppm.o

$(OBJ_DIR)/decode_JPEG.o: $(SRC_DIR)/decode_JPEG.c $(INC_DIR)/decode_JPEG.h $(INC_DIR)/decodage_MCU.h $(INC_DIR)/jpeg_reader.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/decode_JPEG.c -pg -o $(OBJ_DIR)/decode_JPEG.o

$(OBJ_DIR)/huffman.o: $(SRC_DIR)/huffman.c $(INC_DIR)/huffman.h $(INC_DIR)/jpeg_reader.h $(INC_DIR)/bitstream.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/huffman.c -pg -o $(OBJ_DIR)/huffman.o

$(OBJ_DIR)/jpeg_reader.o: $(SRC_DIR)/jpeg_reader.c $(INC_DIR)/jpeg_reader.h $(INC_DIR)/bitstream.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/jpeg_reader.c -pg -o $(OBJ_DIR)/jpeg_reader.o

$(OBJ_DIR)/bitstream.o: $(SRC_DIR)/bitstream.c
	$(CC) $(CFLAGS) -c $(SRC_DIR)/bitstream.c -pg -o $(OBJ_DIR)/bitstream.o



.PHONY: clean

clean:
	rm -f $(TARGET) $(OBJ_FILES)
	rm -f images/*.pgm images/*.ppm
