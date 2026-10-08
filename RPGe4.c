#include <stdio.h>
#include <stdlib.h> // Contiene rand() y srand()
#include <time.h>   // Contiene time()


typedef struct{
    char name[24];
    char atribute[5]; // [0]HP, [1]ataque físico, [2]ataque mágico, [3]defensa física y [4]defensa mágica
    int power;
} Objeto;

typedef int (*FuncionMagia)(void * atacante, void * defensor);

typedef struct{

    char name[50];
    FuncionMagia funcion;

}EntradaMagia;



typedef struct {
    char name[24];
    int HP;
    int danio;
    int physic_Attck;       // Daño base de los ataques físicos
    int magic_Attck;        // Poder de los ataques mágicos
    int physic_Defense;     // Defensa contra ataques físicos
    int magic_Defense;      // Defensa contra ataques mágicos

    int perder_Turno;       // Bandera: 1 si el personaje pierde su siguiente turno, 0 si puede jugar
    int evadir;             // Bandera: 1 si está evadiendo el próximo ataque, 0 si no está activa

    // Variables utilizadas para controlar los efectos temporales de las magias
    int atributo_mod;       // Atributo que fue modificado
    int cant_mod;           // Cantidad que se aumentó o redujo el atributo
    int cant_turnos;        // Número de turnos restantes del efecto

    FuncionMagia magic[3];  // Tres apuntadores a las magias seleccionadas por el personaje
    char * magic_name[3];   // Nombres de las tres magias seleccionadas

    Objeto * inventario[5]; // Apuntadores a los objetos que tiene el personaje
    int num_objects;        // Número actual de objetos almacenados en el inventario
    

} Personaje;

// Asigna una magia del catálogo al personaje y guarda también su nombre
// en la posición correspondiente de su arreglo de magias.
//numero_Magia: cuál magia eligió el jugador del catálogo de 5. Sirve para saber de dónde sacar la función y el nombre.
//posicion_magia: en cuál de sus 3 casillas la va a guardar (0, 1 o 2).
//numero_Magia va de 0 a 4
//posicion_magia va de 0 a 2 
void asignarMagia( Personaje * jugador, int numero_Magia, int posicion_magia, EntradaMagia catalogo_Magias[]){

    EntradaMagia * cat_Temp = catalogo_Magias;
    cat_Temp = cat_Temp + numero_Magia;

    *(jugador->magic_name + posicion_magia )= cat_Temp->name;

    *(jugador->magic + posicion_magia ) = cat_Temp->funcion;


}
