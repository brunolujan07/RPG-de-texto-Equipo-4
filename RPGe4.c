#include <stdio.h>
#include <stdlib.h> // Contiene rand() y srand()
#include <time.h>   // Contiene time()
#include <string.h>

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
    int atributo_mod;       // Atributo que fue modificado -1 ninguno, 0 HP, 1 ataque físico, 2 ataque mágico, 3 defensa física, 4 defensa mágica
    int cant_mod;           // Cantidad que se aumentó o redujo el atributo, 1 por default
    int cant_turnos;        // Número de turnos restantes del efecto

    FuncionMagia magic[3];  // Tres apuntadores a las magias seleccionadas por el personaje
    char * magic_name[3];   // Nombres de las tres magias seleccionadas

    Objeto * inventario[5]; // Apuntadores a los objetos que tiene el personaje
    int num_objects;        // Número actual de objetos almacenados en el inventario
    

} Personaje;


//FUNCIONES

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

//5 FUNCIONES DE MAGIA!!

// Intenta aplicar el hechizo para hacer que el defensor pierda su turno.
// Si el ataque mágico del atacante es mayor o igual a la defensa mágica del defensor,
// activa la bandera 'perder_Turno' en el objetivo y regresa 1. En caso contrario, regresa 0.
int perderTurno(void* atacante, void * defensor){

    //Cast de los punteros genéricos 'void*' a la estructura 'Personaje'
    Personaje * lanza = (Personaje * )atacante;
    Personaje * recibe = (Personaje * )defensor;

    printf("%s lanza el hechizo Rompe-Rodillas. ", lanza->name);
    // Compara el ataque mágico contra la defensa mágica del objetivo
    if(lanza->magic_Attck >= recibe->magic_Defense){
        recibe->perder_Turno = 1;
        printf("Magia (%s) %d vs Defensa (%s) %d - Magia exitosa\n", lanza->name, lanza->magic_Attck, recibe->name, recibe->magic_Defense);
        printf("%s pierde un turno\n", recibe->name);
        return 1;
    }else{
        printf("Magia (%s) %d vs Defensa (%s) %d - Magia falla\n", lanza->name, lanza->magic_Attck, recibe->name, recibe->magic_Defense);
        return 0;
    }
    
}

// Aumenta temporalmente el ataque mágico del atacante en +3 puntos durante 3 turnos.
// Registra la modificación en la estructura para poder revertir el efecto después.
//Siempre regresa 1 porque siempre es válida
int attackbuff(void * atacante, void * defensor){

    Personaje * jugador = (Personaje * ) atacante;
    (void)defensor;

    printf("%s esta preparando la jeringa de Polvo de hadas? ", jugador->name);
    printf("El %s se puso chochos!!\n", jugador->name);
    // Registra los datos del efecto temporal en el personaje
    jugador->atributo_mod = 2; // 2 representa el atributo 'magic_Attck'
    jugador->cant_mod = 3;     // Incremento de +3 puntos de ataque
    jugador->cant_turnos = 3;  // Duración del efecto (3 turnos)

    // Aplica el incremento al ataque mágico actual
    jugador->magic_Attck += jugador->cant_mod;
    printf("El Ataque mágico sube! Ahora vale: %d \n", jugador->magic_Attck);

    return 1;

}

// Reduce permanentemente la defensa física del enemigo dejándolo vulnerable.
// Útil para combinarlo con ataques físicos normales en turnos posteriores.
int rompeArmaduras(void * atacante, void * defensor){

    Personaje * lanza = (Personaje *) atacante;
    Personaje * recibe = (Personaje *) defensor;

    printf("%s lanza el hechizo Desgarra-camisas. ", lanza->name);
    // Compara el ataque mágico contra la defensa mágica
    if(lanza->magic_Attck >= recibe->magic_Defense){
        printf("¡La magia de %s corroe la armadura de %s!\n", lanza->name, recibe->name);
        
        // Reduce la defensa física enemiga
        recibe->physic_Defense -= 3; 
        
        // Evita que la defensa física sea menor a 0
        if(recibe->physic_Defense < 0) recibe->physic_Defense = 0;
        
         printf("Magia (%s) %d vs Defensa (%s) %d - Magia exitosa\n", lanza->name, lanza->magic_Attck, recibe->name, recibe->magic_Defense);
        return 1; // Éxito
    } else {
        printf("Magia (%s) %d vs Defensa (%s) %d - Magia falla\n", lanza->name, lanza->magic_Attck, recibe->name, recibe->magic_Defense);
        return 0; // Fallo
    }
}

// Magia curativa "Electrolit". Reduce el daño acumulado del lanzador.
// Como se aplica a uno mismo, ignora la defensa enemiga y siempre funciona.
int electrolit(void * atacante, void * defensor){
    
    Personaje * jugador = (Personaje *) atacante;
    (void) defensor;

    printf("¡%s se toma un Electrolit!\n", jugador->name);
    
    // Reduce el daño recibido en 9 puntos 
    jugador->danio -= 9;
    
    // Evita que el daño sea menor a 0
    if(jugador->danio < 0) {
        jugador->danio = 0; 
    }
    
    printf("¡%s se puso las pilas!-yessir Daño actual: %d\n",jugador->name, jugador->danio);
    
    return 1; // Siempre exitoso
}

// Magia ofensiva "puff". Hace daño directo a la salud del rival.
// Si tu ataque mágico supera su defensa, significa que aceptó la fumada y recibe el daño.
//Danio no se limita porque eso se decide en el ciclo de batalla
//1 si funciona y 0 si falla
int puff(void * atacante, void * defensor){

    Personaje * lanza = (Personaje *) atacante;
    Personaje * recibe = (Personaje *) defensor;

    printf("%s prepara el hechizo vape?? ", lanza->name);
    // Compara el ataque mágico contra la defensa mágica enemiga
    if(lanza->magic_Attck >= recibe->magic_Defense){
        printf("Magia (%s) %d vs Defensa (%s) %d - Magia exitosa\n", lanza->name, lanza->magic_Attck, recibe->name, recibe->magic_Defense);
        printf("¡%s le pasa un vape sabor cagada a %s!\n", lanza->name, recibe->name);
        // Le revienta los pulmones sumándole daño directo 
        recibe->danio += 9; 
        printf("%s recibe 9 puntos de daño. Vida restante: %d \n", recibe->name,  (recibe->HP - recibe->danio) );
        
        return 1; // Éxito
    } else {
        
        printf("Magia (%s) %d vs Defensa (%s) %d - Magia falla\n", lanza->name, lanza->magic_Attck, recibe->name, recibe->magic_Defense);
        printf("¡%s dijo que no fuma y lo esquivó!\n", recibe->name);
        return 0; // Fallo (la defensa bloqueó el ataque)
    }
}

void imprimirMagias(EntradaMagia catalogo_Magias[]){

    EntradaMagia * catalogo_Temp = catalogo_Magias;
    printf("CATÁLOGO DE MAGIAS!\n");
    for(int i = 0; i < 5; i++){
        printf("\t%d. %s\n", i+1, (catalogo_Temp + i)->name);
    }
}

int pedirMagias(){
    
    int num_magia = 0;
    do
    {
        printf("Elige el número de magia que quieres (1-5)! \n");
        scanf("%d", &num_magia);
        getchar();
    } while (num_magia<1 || num_magia>5);

    return num_magia -1;
    
}


void crearJugador(Personaje * personaje ){

        printf("Ingresa tu nombre valiente juagdor! \n");
        scanf("%23s", personaje->name);
        personaje->HP = 20;
        personaje->danio = 0;
        personaje->physic_Attck = 5;
        personaje->magic_Attck = 5;
        personaje->physic_Defense = 5;
        personaje->magic_Defense = 5;
        personaje->perder_Turno = 0;
        personaje->evadir = 0;
        personaje->atributo_mod = -1;
        personaje->num_objects = 0;
        personaje->cant_mod= 0;
        personaje->cant_turnos= 0;
        Personaje * copia_personaje = personaje;
        for(int i = 0; i<5; i++){
            *(copia_personaje->inventario + i)= NULL;
        }
}

void elegirMagias(Personaje * personaje, EntradaMagia catalogo_Magias[]){
    for(int posicion = 0; posicion<3 ; posicion++){
        imprimirMagias(catalogo_Magias);
        int numero = pedirMagias();
        asignarMagia(personaje,numero, posicion,catalogo_Magias);
        printf("Se escogió la magia : %s \n", *(personaje->magic_name + posicion));
    }

}

//Crear Objetos
void createObject(Objeto *item, char *name, int atribute[5]) {
    
    // Se copia el string directamente al arreglo estático (24 bytes)
    strcpy(item->name, name);
    
    for(int i = 0; i < 5; i++){
        *(item->atribute + i) = *(atribute + i);
    }
    
    // Genera un poder aleatorio del 1 al 5
    item->power = rand() % 5 + 1;
}

void asignarMagiasCPU(Personaje * personaje, EntradaMagia catalogo_Magias[]){

    int magias_elegidas[5] = {0, 0, 0, 0, 0};
    int numero;
    int posicion = 0;

    // Selecciona tres magias aleatorias sin repetir
    while(posicion < 3){

        numero = rand() % 5;

        // Verifica si la magia todavía no ha sido elegida
        if(*(magias_elegidas + numero) == 0){

            // Marca la magia como elegida
            *(magias_elegidas + numero) = 1;

            // Guarda la función y el nombre de la magia en el personaje
            asignarMagia(personaje, numero, posicion, catalogo_Magias);

            printf("%s aprende la magia: %s\n",
                   personaje->name,
                   *(personaje->magic_name + posicion));

            posicion++;
        }
    }
}


void crearObjetosInicialesCPU(Personaje * personaje, int cantidad){

    // Arreglo de nombres vinculados al atributo que van a mejorar. 
    // El índice coincide con el arreglo de atributos (0 a 4).
    char * nombres_objetos[5] = {
        "Objeto 1",       
        "Objeto 2",      
        "Objeto 3",         
        "Objeto 4",     
        "Objeto 5"   
    };

    for(int i = 0; i < cantidad; i++){

        // Reserva memoria dinámica para el objeto (Malloc)
        Objeto * objeto = malloc(sizeof(Objeto));

        if(objeto == NULL){
            printf("Error al reservar memoria para el objeto.\n");
            return;
        }

        // 1. Selecciona aleatoriamente el atributo PRIMERO (0 al 4)
        int atributo = rand() % 5;

        // 2. Le asigna el nombre sacándolo del arreglo usando el número aleatorio
        strcpy(objeto->name, *(nombres_objetos + atributo));

        // Inicializa los indicadores de los cinco atributos
        for(int j = 0; j < 5; j++){
            *(objeto->atribute + j) = 0;
        }

        // Marca el atributo modificado
        *(objeto->atribute + atributo) = 1;

        // Asigna un poder aleatorio entre 1 y 5
        objeto->power = rand() % 5 + 1;

        // Guarda el objeto en el inventario
        *(personaje->inventario + personaje->num_objects) = objeto;
        personaje->num_objects++;

        // Aplica la mejora al atributo correspondiente
        if(atributo == 0){
            personaje->HP += objeto->power;
        }else if(atributo == 1){
            personaje->physic_Attck += objeto->power;
        }else if(atributo == 2){
            personaje->magic_Attck += objeto->power;
        }else if(atributo == 3){
            personaje->physic_Defense += objeto->power;
        }else{
            personaje->magic_Defense += objeto->power;
        }

        printf("%s recibe %s: poder +%d\n",
               personaje->name, objeto->name, objeto->power);
    }
}

void crearCPU(Personaje * personaje,char * nombre,int HP,int danio,int physic_Attck,int magic_Attck,int physic_Defense,int magic_Defense){

    // Asigna el nombre y los atributos principales del enemigo
    strcpy(personaje->name, nombre);
    personaje->HP = HP;
    personaje->danio = danio;
    personaje->physic_Attck = physic_Attck;
    personaje->magic_Attck = magic_Attck;
    personaje->physic_Defense = physic_Defense;
    personaje->magic_Defense = magic_Defense;

    // Inicializa las banderas de estado del personaje
    personaje->perder_Turno = 0;
    personaje->evadir = 0;

    // Inicializa las variables de efectos temporales
    personaje->atributo_mod = -1;
    personaje->cant_mod = 0;
    personaje->cant_turnos = 0;

    // Inicializa el inventario vacío
    personaje->num_objects = 0;

    Objeto ** objeto_temp = personaje->inventario;

    for(int i = 0; i < 5; i++){
        *(objeto_temp + i) = NULL;
    }

    // Inicializa los tres espacios de magia
    FuncionMagia * magia_temp = personaje->magic;
    char ** nombre_magia_temp = personaje->magic_name;

    for(int i = 0; i < 3; i++){
        *(magia_temp + i) = NULL;
        *(nombre_magia_temp + i) = NULL;
    }

    
}

//INICIALIZACIÓN
void inicializarCPUs(Personaje * jugadores, EntradaMagia catalogo_Magias[]){

    crearCPU(jugadores + 1, "Enemigo facil",20, 0, 5, 5, 5, 5);
    asignarMagiasCPU(jugadores + 1, catalogo_Magias);

    crearCPU(jugadores + 2, "Enemigo intermedio",25, 0, 7, 7, 7, 7);
    crearObjetosInicialesCPU(jugadores + 2, 2);
    asignarMagiasCPU(jugadores + 2, catalogo_Magias);

    crearCPU(jugadores + 3, "Enemigo dificil",30, 0, 9, 9, 9, 9);
    crearObjetosInicialesCPU(jugadores + 3, 3);
    asignarMagiasCPU(jugadores + 3, catalogo_Magias);

    crearCPU(jugadores + 4, "Jefe Final",40, 0, 12, 12, 10, 10);
    crearObjetosInicialesCPU(jugadores + 4, 5);
    asignarMagiasCPU(jugadores + 4, catalogo_Magias);
}


int main(){

    srand(time(NULL));

    EntradaMagia catalogo_Magias[5] = {
    {"Rompe-Rodillas", perderTurno},        // Pierde turno
    {"Chochos", attackbuff},         // Sube tu ataque mágico (obligatorio)
    {"Electrolit", electrolit},             // Te cura
    {"Puff", puff},                   // Daño directo a los pulmones
    {"Desgarra-camisas", rompeArmaduras}    // Baja defensa física
    };

    // Arreglo de cinco personajes: jugador y cuatro enemigos
    Personaje jugadores[5];

    // Crea al jugador humano
    crearJugador(jugadores);
    elegirMagias(jugadores, catalogo_Magias);

    // Inicializa los enemigos con sus atributos por dificultad
    inicializarCPUs(jugadores, catalogo_Magias);

    return 0;
}

    