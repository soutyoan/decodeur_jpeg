#include "../include/huffman.h"

#define LOG_LOAD 0

#include <stdlib.h>
#include <stdio.h>

// on stocke la table de huffman en arbre directement
struct huff_table {
    // chaque huff_table a un tableau avec ses deux fils si ce n'est pas une feuille (sinon les fils sont NULL)
    struct huff_table **fils;
    uint8_t valeur;
    bool set;
};

// Affiche les valeurs d'une huff_table en ligne pour le debug
void affiche_huff_table(struct huff_table *table) {
    if(table -> set) printf("%x ", table -> valeur);
    if(table -> fils[0] != NULL) affiche_huff_table(table -> fils[0]);
    if(table -> fils[1] != NULL) affiche_huff_table(table -> fils[1]);
}

// renvoie une huff_table vide (pas de fils, pas de valeurs)
struct huff_table *huff_table_vide() {
    struct huff_table *table_cree = malloc(sizeof(struct huff_table));
    table_cree -> fils = malloc(2*sizeof(struct huff_table*));
    table_cree -> fils[0] = NULL;
    table_cree -> fils[1] = NULL;
    table_cree -> valeur = 0;
    table_cree -> set = false;
    if(table_cree -> fils == NULL) {
        fprintf(stderr, "Erreur lors de la creation d'un noeud dans l'arbre de Huffman\n");
        exit(1);
    }

    return table_cree;
}

// On peut discruter de l'utilite de cette fonction mais bon, affecter un boolean a chaque fois... pfiou
void set_valeur_noeud(struct huff_table *noeud, uint8_t valeur) {
    noeud -> valeur = valeur;
    noeud -> set = true;
}

// Renvoie la profondeur maximale des codes dans l'arbre
uint8_t calcule_etage_max(uint8_t *nb_valeurs) {
    uint8_t dernier_etage_non_nul = 0;
    for(uint8_t i = 0; i < 16; i ++)
        if(nb_valeurs[i] != 0)
            dernier_etage_non_nul = i;
    return dernier_etage_non_nul +1;
}

// Fonction qui renvoie le premier noeud vide a la profondeur p (NULL sinon)
struct huff_table *prochain_noeud_vide(struct huff_table **ensemble_noeud_etage_courant, uint8_t taille_ensemble_noeud_etage_courant) {
    for(uint8_t i = 0; i < taille_ensemble_noeud_etage_courant; i ++) {
        for(uint8_t j = 0; j < 2; j ++)
            if(ensemble_noeud_etage_courant[i] -> fils[j] == NULL) {
                ensemble_noeud_etage_courant[i] -> fils[j] = huff_table_vide();
                return ensemble_noeud_etage_courant[i] -> fils[j];
            }
    }
    return NULL;
}

// On a deja lut les 3 premiers octets dans jpeg_reader on commence directement
// par lire la repartion des valeurs.
struct huff_table *load_huffman_table(struct bitstream *stream, uint16_t *nb_byte_read) {
    struct huff_table *nouvelle_table = huff_table_vide();
    *nb_byte_read = 0;

    uint32_t byte = 0;
    uint8_t nb_valeurs[16];
    // on lit le nombres de valeurs par profondeur de 1 a 16
    for(uint8_t i = 0; i < 16; i ++) {
        *nb_byte_read += read_bitstream(stream, 8, &byte, false)/8;
        nb_valeurs[i] = byte;
    }
    if(LOG_LOAD > 0) { // affichage de la repartition
        for(uint8_t i = 0; i < 16; i ++)
            printf("%u ", nb_valeurs[i]);
        printf("\n");
    }

    // On lit ensuite les valeurs en construisant recursivement l'arbre de Huffman
    struct huff_table **ensemble_noeud_courant = malloc(sizeof(struct huff_table*));
    // on doit faire un malloc puisqu'on fait un free dans le calcul de l'etage suivant
    *ensemble_noeud_courant = nouvelle_table;
    uint16_t taille_ensemble_noeud_etage_courant = 1;
    struct huff_table *nouveau_noeud;
    uint8_t etage_max = calcule_etage_max(nb_valeurs); // on fait ca pour pas avoir une table geante remplie de valeur NULL
    // pour chaque etage
    for(uint8_t i = 0; i < etage_max; i ++) {
        // on place dans les premieres feuilles les prochaines valeurs
        for(uint8_t j = 0; j < nb_valeurs[i]; j ++) {
            *nb_byte_read += read_bitstream(stream, 8, &byte, false)/8;
            nouveau_noeud = prochain_noeud_vide(ensemble_noeud_courant, taille_ensemble_noeud_etage_courant);
            if(nouveau_noeud != NULL) set_valeur_noeud(nouveau_noeud, (uint8_t) byte);
            else printf("Erreur lors de la creation d'une table de Huffman\n");
        }

        if(i != etage_max -1) {
            // on calcule le nombre de noeuds vide de l'etage courant
            uint16_t nb_noeuds_vide = 2 * taille_ensemble_noeud_etage_courant - nb_valeurs[i];

            struct huff_table **ensemble_noeud_suivant = malloc(sizeof(struct huff_table*)*nb_noeuds_vide);
            uint16_t k = 0;
            // on complete l'etage avec des noeuds vide pour acceuillir les valeurs suivantes et on construit notre etage suivant
            struct huff_table *noeud_pour_completer_etage = prochain_noeud_vide(ensemble_noeud_courant, taille_ensemble_noeud_etage_courant);
            while(noeud_pour_completer_etage != NULL) {
                ensemble_noeud_suivant[k] = noeud_pour_completer_etage;
                noeud_pour_completer_etage = prochain_noeud_vide(ensemble_noeud_courant, taille_ensemble_noeud_etage_courant);
                k ++;
            }
            // on remplace notre etage
            free(ensemble_noeud_courant);
            ensemble_noeud_courant = ensemble_noeud_suivant;
            taille_ensemble_noeud_etage_courant = nb_noeuds_vide;
        }
    }

    if(LOG_LOAD > 0) {
        affiche_huff_table(nouvelle_table);
        printf("\n");
    }

    free(ensemble_noeud_courant);
    return nouvelle_table;
}

// on parcourt l'arbre jusqu'a obtenir une valeur, ou pas.
int8_t next_huffman_value(struct huff_table *table, struct bitstream *stream) {
    uint32_t byte = 0;
    while(!table -> set) {
        // on lit le prochain bit
        read_bitstream(stream, 1, &byte, true);
        // on avance dans l'arbre (on verifie si on a bien 0 ou 1 mais c'est presque du troll, pendant le developpement on a recu 3 en lisant 1 bit une fois)
        if((byte != 0 && byte != 1) || table -> fils[byte] == NULL) {
            printf("Caractere de Huffman invalide dans le bitstream, lu %u\n", byte);
            exit(1);
        } else table = table -> fils[byte];
    }
    return table -> valeur;
}

// On libere tout recursivement
extern void free_huffman_table(struct huff_table *table) {
    if(table != NULL) {
        free_huffman_table(table -> fils[1]);
        free_huffman_table(table -> fils[0]);
        free(table -> fils);
        free(table);
    }
}
