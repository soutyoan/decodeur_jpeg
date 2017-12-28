#include "../include/decode_JPEG.h"
#include "../include/image_ppm.h"

#include <string.h>

/*
    INVADER : OK \o/
    GRIS : OK \o/
    BISOU : OK \o/
    ZIG-ZAG : OK \o/
    THUMBS : OK \o/
    HORIZONTAL : OK \o/
    VERTICAL : OK \o/
    SAUH THE SHEEP : \o_ OK _o/ \o/
    COMPLEXITE : OK \o/
*/

void verifie_extension(const char *filename) {
    const char *extension = strrchr(filename, '.');
    // Si on ne trouve pas de point ou si les deux pointeurs sont egaux (ie le nom commence par un point)
    if(extension == NULL || extension == filename) {
        fprintf(stderr, "Nom de fichier invalide\n");
        exit(1);
    }
    if(strcmp(extension, ".jpg") && strcmp(extension, ".jpeg") && strcmp(extension, ".JPG")) {
        fprintf(stderr, "Extension du fichier invalide, attendu : .jpg .jpeg .JPG\n");
        exit(1);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
    /* Si y'a pas au moins un argument en ligne de commandes, on
     * boude. */
        fprintf(stderr, "Usage: %s fichier.jpeg\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Super on peut mettre plusieurs arguments a la suite !
    for(uint8_t i = 1; i < argc; i ++) {
        /* On recupere le nom du fichier JPEG sur la ligne de commande. */
        const char *filename = argv[i];
        verifie_extension(filename);

        /* On cree un jpeg_desc qui permettra de lire ce fichier. */
        struct jpeg_desc *jdesc = read_jpeg(filename);
        /* On recupere le flux des donnees brutes a partir du descripteur. */
        struct bitstream *stream = get_bitstream(jdesc);

        // On initialise notre structure d'image
        struct image_jpeg *image = prepare_lecture_MCUs(jdesc);
        // On decode ensuite le flux pour obtenir une bitmap
        printf("Début du decodage de %s\n", filename);
        image = decode_image_jpeg(image, jdesc, stream);
        //
        // Et on cree le fichier de sortie
        create_image_ppm(image, argv[i]);

        /* Nettoyage de printemps : close_jpeg ferme aussi le bitstream
         * (voir Annexe C du sujet). */
        close_jpeg(jdesc);
        free_image_jpeg(image);
    }
    /* On se congratule. */
    return EXIT_SUCCESS;
}
