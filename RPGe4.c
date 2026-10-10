#include <stdio.h>
#include <stdlib.h> // Contiene rand() y srand()
#include <time.h>   // Contiene time()
#include <string.h>

typedef struct{
    char name[24];
    int atribute; // 0 HP, 1 ataque físico, 2 ataque mágico, 3 defensa física, 4 defensa mágica
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
    int mod_batalla[5];     // Modificadores que duran toda la batalla (se reinician al empezar cada una)

    FuncionMagia magic[3];  // Tres apuntadores a las magias seleccionadas por el personaje
    char * magic_name[3];   // Nombres de las tres magias seleccionadas

    Objeto * inventario[5]; // Apuntadores a los objetos que tiene el personaje
    int num_objects;        // Número actual de objetos almacenados en el inventario
    
} Personaje;


///===================================FUNCIONES GENERALES===================================

// Asigna una magia del catálogo al personaje y guarda también su nombre
// en la posición correspondiente de su arreglo de magias.
//numero_Magia: cuál magia eligió el jugador del catálogo de 5. Sirve para saber de dónde sacar la función y el nombre.
//posicion_magia: en cuál de sus 3 casillas la va a guardar (0, 1 o 2).
//numero_Magia va de 0 a 4
//posicion_magia va de 0 a 2 
void asignarMagia( Personaje * jugador, int numero_Magia, int posicion_magia, EntradaMagia catalogo_Magias[]){
    EntradaMagia * cat_Temp = catalogo_Magias;
    cat_Temp = cat_Temp + numero_Magia;

    *(jugador->magic_name + posicion_magia ) = cat_Temp->name;

    *(jugador->magic + posicion_magia ) = cat_Temp->funcion;
}

// Regresa el valor efectivo de un atributo: base + objetos + efectos de batalla + efecto temporal
// atributo: 0 HP, 1 ataque físico, 2 ataque mágico, 3 defensa física, 4 defensa mágica
int statActual(Personaje * p, int atributo) {
    int total;
 
    // Valor base
    if(atributo == 0)      total = p->HP;
    else if(atributo == 1) total = p->physic_Attck;
    else if(atributo == 2) total = p->magic_Attck;
    else if(atributo == 3) total = p->physic_Defense;
    else                   total = p->magic_Defense;
 
    // Bonus de los objetos equipados
    for(int i = 0; i < p->num_objects; i++){
        Objeto * o = *(p->inventario + i);
        if(o->atribute == atributo){
            total += o->power;
        }
    }
 
    // Modificadores de toda la batalla (Desgarra-camisas)
    total += *(p->mod_batalla + atributo);
 
    // Efecto temporal activo (Chochos)
    if(p->cant_turnos > 0 && p->atributo_mod == atributo){
        total += p->cant_mod;
    }
 
    if(total < 0) total = 0;
    return total;
}

char * nombreAtributo(int atributo) {
    if(atributo == 0) return "HP";
    if(atributo == 1) return "Ataque fisico";
    if(atributo == 2) return "Ataque magico";
    if(atributo == 3) return "Defensa fisica";
    return "Defensa magica";
}
 
// Reinicia los efectos de batalla. Se llama al empezar cada pelea
void reiniciarEfectos(Personaje * p) {
    for(int i = 0; i < 5; i++){
        *(p->mod_batalla + i) = 0;
    }
    p->danio = 0;
    p->atributo_mod = -1;
    p->cant_mod = 0;
    p->cant_turnos = 0;
    p->perder_Turno = 0;
    p->evadir = 0;
}

// Avanza el contador del efecto temporal. Se llama al terminar el turno de cada personaje
void avanzarEfectos(Personaje * p){
    if(p->cant_turnos > 0){
        p->cant_turnos--;
        if(p->cant_turnos == 0){
            p->atributo_mod = -1;
            p->cant_mod = 0;
            printf("El efecto de %s termino\n", p->name);
        }
    }
}

// Pide un número entre min y max, igual que pedirMagias
int pedirNumero(int min, int max){
    int num = 0;
    do{
        printf("Elige una opcion (%d-%d): ", min, max);
        scanf("%d", &num);
        getchar();
    }while(num < min || num > max);
    return num;
}

//===================================FUNCIONES DE MAGIA===================================

// Probabilidad de que una magia funcione: 50% base, +/-10% por cada punto de diferencia,
// limitado entre 10% y 90%. Asi nunca es imposible ni segura
int magiaExitosa(int magia, int defensa){
    int probabilidad = 50 + 10 * (magia - defensa);
    if(probabilidad < 10) probabilidad = 10;
    if(probabilidad > 90) probabilidad = 90;
    return rand() % 100 < probabilidad;
}

// Intenta aplicar el hechizo para hacer que el defensor pierda su turno.
// Si el ataque mágico del atacante es mayor o igual a la defensa mágica del defensor,
// activa la bandera 'perder_Turno' en el objetivo y regresa 1. En caso contrario, regresa 0.
int perderTurno(void * atacante, void * defensor){

    Personaje * lanza = (Personaje *) atacante;
    Personaje * recibe = (Personaje *) defensor;

    int magia = statActual(lanza, 2);
    int defensa = statActual(recibe, 4);

    printf("%s lanza el hechizo Rompe-Rodillas. ", lanza->name);
    if(magiaExitosa(magia, defensa)) {
        recibe->perder_Turno = 1;
        printf("Magia (%s) %d vs Defensa (%s) %d - Magia exitosa\n", lanza->name, magia, recibe->name, defensa);
        printf("%s pierde un turno\n", recibe->name);
        return 1;
    }else{
        printf("Magia (%s) %d vs Defensa (%s) %d - Magia falla\n", lanza->name, magia, recibe->name, defensa);
        return 0;
    }
}

// Aumenta temporalmente el ataque mágico del atacante en +3 puntos durante 3 turnos.
// Registra la modificación en la estructura para poder revertir el efecto después.
//Siempre regresa 1 porque siempre es válida
int attackbuff(void * atacante, void * defensor){

    Personaje * jugador = (Personaje *) atacante;
    (void) defensor;

    printf("%s esta preparando la jeringa de Polvo de hadas? ", jugador->name);
    printf("El %s se puso chochos!!\n", jugador->name);

    jugador->atributo_mod = 2; // 2 representa el atributo 'magic_Attck'
    jugador->cant_mod = 3;     // Incremento de +3 puntos de ataque
    jugador->cant_turnos = 3;  // Duración del efecto (3 turnos)

    printf("El Ataque magico sube! Ahora vale: %d \n", statActual(jugador, 2));

    return 1;
}

// Reduce permanentemente la defensa física del enemigo dejándolo vulnerable.
// Útil para combinarlo con ataques físicos normales en turnos posteriores.
int rompeArmaduras(void * atacante, void * defensor){

    Personaje * lanza = (Personaje *) atacante;
    Personaje * recibe = (Personaje *) defensor;

    int magia = statActual(lanza, 2);
    int defensa = statActual(recibe, 4);

    printf("%s lanza el hechizo Desgarra-camisas. ", lanza->name);
    if(magiaExitosa(magia, defensa)) {
        printf("Magia (%s) %d vs Defensa (%s) %d - Magia exitosa\n", lanza->name, magia, recibe->name, defensa);
        printf("La magia de %s corroe la armadura de %s!\n", lanza->name, recibe->name);

        // Solo reduce si todavía hay defensa física que quitar
        if(statActual(recibe, 3) > 0){
            *(recibe->mod_batalla + 3) -= 3;
        }
        printf("Defensa fisica de %s: %d\n", recibe->name, statActual(recibe, 3));

        return 1; // Éxito
    } else {
        printf("Magia (%s) %d vs Defensa (%s) %d - Magia falla\n", lanza->name, magia, recibe->name, defensa);
        return 0; // Fallo
    }
}

// Magia curativa "Electrolit". Reduce el daño acumulado del lanzador.
// Como se aplica a uno mismo, ignora la defensa enemiga y siempre funciona.
int electrolit(void * atacante, void * defensor){
    
    Personaje * jugador = (Personaje *) atacante;
    (void) defensor;

    printf("%s se toma un Electrolit!\n", jugador->name);
    
    // Reduce el daño recibido en 9 puntos 
    jugador->danio -= 9;
    
    // Evita que el daño sea menor a 0
    if(jugador->danio < 0) {
        jugador->danio = 0; 
    }
    
    printf("%s se puso las pilas!-yessir Danio actual: %d\n",jugador->name, jugador->danio);
    
    return 1; // Siempre exitoso
}

// Magia ofensiva "puff". Hace daño directo a la salud del rival.
// Si tu ataque mágico supera su defensa, significa que aceptó la fumada y recibe el daño.
//Danio no se limita porque eso se decide en el ciclo de batalla
//1 si funciona y 0 si falla
int puff(void * atacante, void * defensor){

    Personaje * lanza = (Personaje *) atacante;
    Personaje * recibe = (Personaje *) defensor;

    int magia = statActual(lanza, 2);
    int defensa = statActual(recibe, 4);

    printf("%s prepara el hechizo vape?? ", lanza->name);
    if(magiaExitosa(magia, defensa)) {
        printf("Magia (%s) %d vs Defensa (%s) %d - Magia exitosa\n", lanza->name, magia, recibe->name, defensa);
        printf("%s le pasa un vape sabor cagada a %s!\n", lanza->name, recibe->name);

        recibe->danio += 9;
        int vida_res = (statActual(recibe, 0) - recibe->danio) < 0 ? 0 : statActual(recibe, 0) - recibe->danio;
        printf("%s recibe 9 puntos de danio. Vida restante: %d \n", recibe->name, vida_res < 0 ? 0 : vida_res);

        return 1; // Éxito
    } else {
        printf("Magia (%s) %d vs Defensa (%s) %d - Magia falla\n", lanza->name, magia, recibe->name, defensa);
        printf("%s dijo que no fuma y lo esquivo!\n", recibe->name);
        return 0; // Fallo
    }
}

void imprimirMagias(EntradaMagia catalogo_Magias[]){

    EntradaMagia * catalogo_Temp = catalogo_Magias;
    printf("CATALOGO DE MAGIAS!\n");
    for(int i = 0; i < 5; i++){
        printf("\t%d. %s\n", i+1, (catalogo_Temp + i)->name);
    }
}

void elegirMagias(Personaje * personaje, EntradaMagia catalogo_Magias[]){
    for(int posicion = 0; posicion<3 ; posicion++){
        imprimirMagias(catalogo_Magias);
        int numero = pedirNumero(1,5) - 1;
        asignarMagia(personaje,numero, posicion,catalogo_Magias);
        printf("Se escogio la magia : %s \n", *(personaje->magic_name + posicion));
    }

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

//===================================OBJETOS===================================

// Reserva memoria para un objeto que mejora el atributo indicado, con poder aleatorio.
// Regresa NULL si malloc falla
Objeto * crearObjetoAleatorio(int atributo, int nivel){
    // El índice del nombre coincide con el atributo que mejora
    char * nombres[5] = {
        "Caldo de la abuela",   // 0 HP
        "Chancla de la jefa",   // 1 ataque físico
        "Cafe de Oxxo",         // 2 ataque mágico
        "Casco de moto",        // 3 defensa física
        "Limpia con huevo"      // 4 defensa mágica
    };
 
    Objeto * o = malloc(sizeof(Objeto));
    if(o == NULL) return NULL;
 
    strcpy(o->name, *(nombres + atributo));
    o->atribute = atributo;
    o->power = rand() % 3 + 1 + nivel; 

    return o;
}
 
void imprimirObjeto(Objeto * o, int numero){
    printf("\t%d. %s (+%d %s)\n", numero, o->name, o->power, nombreAtributo(o->atribute));
}
 
void imprimirInventario(Personaje * p){
    printf("Inventario de %s:\n", p->name);
    for(int i = 0; i < p->num_objects; i++){
        imprimirObjeto(*(p->inventario + i), i + 1);
    }
}
 
// Quita un objeto del inventario, libera su memoria y recorre los demás.
void eliminarObjeto(Personaje * p, int indice){
    Objeto * o = *(p->inventario + indice);
    int atributo = o->atribute;
    int antes = statActual(p, atributo);
 
    printf("%s tira %s.\n", p->name, o->name);
    free(o);
 
    for(int i = indice; i < p->num_objects - 1; i++){
        *(p->inventario + i) = *(p->inventario + i + 1);
    }
    p->num_objects--;
    *(p->inventario + p->num_objects) = NULL;
 
    printf("%s: %d -> %d\n", nombreAtributo(atributo), antes, statActual(p, atributo));
}
 
// Agrega un objeto al inventario. Si está lleno, el jugador elige cuál tirar primero.
// El beneficio se aplica solo, porque statActual suma los objetos del inventario
void agregarObjeto(Personaje * p, Objeto * o){
    if(p->num_objects == 5){
        printf("\nTu inventario esta lleno. Elige un objeto para tirar:\n");
        imprimirInventario(p);
        int indice = pedirNumero(1, 5) - 1;
        eliminarObjeto(p, indice);
    }
 
    int atributo = o->atribute;
    int antes = statActual(p, atributo);
 
    *(p->inventario + p->num_objects) = o;
    p->num_objects++;
 
    printf("%s equipa %s. %s: %d -> %d\n", p->name, o->name,
           nombreAtributo(atributo), antes, statActual(p, atributo));
}
 
// Da objetos iniciales a un enemigo. Se guardan en su inventario y statActual los suma
void crearObjetosInicialesCPU(Personaje * personaje, int cantidad){
 
    for(int i = 0; i < cantidad; i++){
 
        Objeto * objeto = crearObjetoAleatorio(rand() % 5, 1);
 
        if(objeto == NULL){
            printf("Error al reservar memoria para el objeto.\n");
            return;
        }
 
        *(personaje->inventario + personaje->num_objects) = objeto;
        personaje->num_objects++;
 
        printf("%s recibe %s: +%d %s\n",
               personaje->name, objeto->name, objeto->power, nombreAtributo(objeto->atribute));
    }
}
 
// Genera 4 objetos con atributos distintos, el jugador elige 2 y los demás se liberan
void recompensas(Personaje * jugador, int nivel){
    Objeto * opciones[4];
    int usados[5] = {0, 0, 0, 0, 0};
    int creados = 0;
 
    // Crea 4 objetos sin repetir atributo, igual que asignarMagiasCPU
    while(creados < 4){
        int atributo = rand() % 5;
        if(*(usados + atributo) == 0){
            *(usados + atributo) = 1;
            *(opciones + creados) = crearObjetoAleatorio(atributo, nivel);
 
            if(*(opciones + creados) == NULL){
                printf("Error al reservar memoria para los objetos.\n");
                for(int i = 0; i < creados; i++) free(*(opciones + i));
                return;
            }
            creados++;
        }
    }
 
    printf("\nEntre los escombros de la pelea encuentras 4 objetos. Solo puedes cargar 2.\n");
 
    for(int elegidos = 0; elegidos < 2; elegidos++){
        printf("\nObjetos disponibles:\n");
        for(int i = 0; i < 4; i++){
            if(*(opciones + i) != NULL){
                imprimirObjeto(*(opciones + i), i + 1);
            }
        }
 
        int eleccion = pedirNumero(1, 4) - 1;
        while(*(opciones + eleccion) == NULL){
            printf("Ese ya lo tomaste.\n");
            eleccion = pedirNumero(1, 4) - 1;
        }
 
        agregarObjeto(jugador, *(opciones + eleccion));
        *(opciones + eleccion) = NULL;   // Ya es del inventario, aquí no se libera
    }
 
    // Libera los objetos que no se eligieron
    for(int i = 0; i < 4; i++){
        if(*(opciones + i) != NULL){
            printf("%s se queda tirado en la banqueta.\n", (*(opciones + i))->name);
            free(*(opciones + i));
            *(opciones + i) = NULL;
        }
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
 
    // Inicializa el inventario vacío y los modificadores de batalla
    personaje->num_objects = 0;
 
    for(int i = 0; i < 5; i++){
        *(personaje->inventario + i) = NULL;
        *(personaje->mod_batalla + i) = 0;
    }
 
    // Inicializa los tres espacios de magia
    for(int i = 0; i < 3; i++){
        *(personaje->magic + i) = NULL;
        *(personaje->magic_name + i) = NULL;
    }
}

///===================================INICIALIZACION===================================
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
            *(personaje->mod_batalla + i) = 0;
            *(copia_personaje->inventario + i)= NULL;
        }
}

void inicializarCPUs(Personaje * jugadores, EntradaMagia catalogo_Magias[]){

    crearCPU(jugadores + 1, "Borracho de la esquina", 20, 0, 5, 5, 5, 5);
    asignarMagiasCPU(jugadores + 1, catalogo_Magias);

    crearCPU(jugadores + 2, "El Cadenero", 25, 0, 7, 7, 7, 7);
    crearObjetosInicialesCPU(jugadores + 2, 2);
    asignarMagiasCPU(jugadores + 2, catalogo_Magias);

    crearCPU(jugadores + 3, "El Viene Viene", 30, 0, 9, 9, 9, 9);
    crearObjetosInicialesCPU(jugadores + 3, 3);
    asignarMagiasCPU(jugadores + 3, catalogo_Magias);

    crearCPU(jugadores + 4, "Cajero del OXXO", 40, 0, 12, 12, 10, 10);
    crearObjetosInicialesCPU(jugadores + 4, 5);
    asignarMagiasCPU(jugadores + 4, catalogo_Magias);
}


///===================================LORE===================================

// Espera a que el jugador presione Enter para continuar
void pausa(){
    printf("\n[Presiona Enter para continuar]");
    while(getchar() != '\n');
}

void separador(){
    printf("\n==================================================\n\n");
}

// Historia inicial, se muestra una sola vez al empezar el juego
void introduccion(Personaje * jugador){
    separador();
    printf("Domingo, 9 de la maniana.\n");
    printf("Despiertas en un sillon que no es tuyo, con un solo zapato y sin cartera.\n");
    printf("La cruda te pega como camion de ruta en hora pico.\n\n");
    printf("Solo existe una cura: un Electrolit de coco bien frio.\n");
    printf("Pero corre el rumor... queda UNO SOLO en toda la colonia.\n");
    printf("Esta en el refri del fondo del OXXO de la esquina.\n\n");
    printf("Entre tu y ese Electrolit hay cuatro obstaculos.\n");
    printf("%s, toma tus magias. Hoy no se muere nadie... mas que tu higado.\n", jugador->name);
    separador();
    pausa();
}

// Se muestra antes de cada pelea. nivel va de 1 a 4
void loreAntesDeBatalla(int nivel, Personaje * jugador, Personaje * enemigo){
    separador();
    switch(nivel){
        case 1:
            printf("NIVEL 1 - La banqueta\n\n");
            printf("Sales de la casa tambaleandote. Alguien te bloquea el paso.\n");
            printf("%s: \"Traes pa'l camion, compa? Nomas me faltan 300 pesos.\"\n", enemigo->name);
            printf("No se va a mover. %s, preparate.\n", jugador->name);
            break;
 
        case 2:
            printf("NIVEL 2 - La calle principal\n\n");
            printf("Llegas frente al antro que nunca cierra. Hay una fila de tres cuadras.\n");
            printf("%s te detiene con una mano: \"Tu no pasas.\"\n", enemigo->name);
            printf("Tu no quieres entrar. Pero al parecer la banqueta tambien es suya.\n");
            break;
 
        case 3:
            printf("NIVEL 3 - El estacionamiento\n\n");
            printf("A una cuadra del OXXO aparece el mas temido de la colonia.\n");
            printf("%s: \"Yo se lo cuido, joven.\"\n", enemigo->name);
            printf("Tu no traes carro. Dicen que nadie ha pasado sin darle diez pesos.\n");
            printf("Tu solo traes un chicle masticado.\n");
            break;
 
        case 4:
            printf("NIVEL FINAL - El mostrador\n\n");
            printf("Las puertas automaticas se abren. Ahi esta el Electrolit, brillando como el Santo Grial.\n");
            printf("Pero detras del mostrador te espera %s.\n", enemigo->name);
            printf("Es el del turno nocturno. Nunca se fue a su casa. Nadie sabe si sigue vivo.\n\n");
            printf("%s: \"Buenos dias. Gusta agregar unas papas? Recarga? Redondeamos?\"\n", enemigo->name);
            printf("Cada pregunta te quita un pedazo del alma. Esta es la batalla final.\n");
            break;
    }
    separador();
    pausa();
}

// Se muestra al ganar cada pelea. En el nivel 4 es el final del juego
void loreVictoria(int nivel, Personaje * jugador, Personaje * enemigo){
    separador();
    switch(nivel){
        case 1:
            printf("%s se queda dormido abrazando un poste.\n", enemigo->name);
            printf("En su bolsa encuentras medio chicle. No sirve de nada, pero ahora es tuyo.\n");
            printf("Sigues caminando...\n");
            break;
 
        case 2:
            printf("%s se retira a llorar detras de su cadena.\n", enemigo->name);
            printf("Tu cruda empeora. Ves doble. Ahora hay dos OXXOs a lo lejos.\n");
            printf("Caminas hacia el de la izquierda. Esperas que sea el real.\n");
            break;
 
        case 3:
            printf("%s te avienta su franela roja en senial de respeto.\n", enemigo->name);
            printf("\"Pasele, joven. Usted si es de la colonia.\"\n");
            printf("Frente a ti, el letrero rojo y amarillo. Ya casi.\n");
            break;
 
        case 4:
            printf("%s cae sobre la maquina de hot dogs.\n", enemigo->name);
            printf("\"...le faltaron... dos centavos... de redondeo...\" susurra antes de desmayarse.\n\n");
            printf("Abres el refri. Tomas el Electrolit. Te lo acabas de un solo trago.\n");
            printf("La cruda desaparece. Los pajaritos cantan. El sol ya no quema.\n\n");
            printf("FELICIDADES %s. Sobreviviste al domingo.\n\n", jugador->name);
            printf("...maniana tienes clase a las 7.\n");
            break;
    }
    separador();
    pausa();
}

// Se muestra si el jugador pierde cualquier pelea
void loreDerrota(Personaje * jugador, Personaje * enemigo){
    separador();
    printf("%s cae al piso.\n", jugador->name);
    printf("Mientras pierdes el conocimiento, %s te mira con lastima.\n\n", enemigo->name);
    printf("Despiertas en el mismo sillon. Un solo zapato. Sin cartera.\n");
    printf("Domingo, 9 de la maniana. Otra vez.\n\n");
    printf("GAME OVER\n");
    separador();
}


///===================================BATALLA===================================

// Vida restante de un personaje: HP efectivo (base + objetos) menos el daño recibido
int vida(Personaje * p){
    return statActual(p, 0) - p->danio;
}
 
// Muestra la vida y stats actuales de ambos personajes
void mostrarEstado(Personaje * jugador, Personaje * enemigo){
    printf("\n--------------------------------------------------\n");
    printf("%-22s Vida: %2d/%2d  AtkF: %2d  AtkM: %2d  DefF: %2d  DefM: %2d\n",
           jugador->name, vida(jugador), statActual(jugador, 0),
           statActual(jugador, 1), statActual(jugador, 2), statActual(jugador, 3), statActual(jugador, 4));
    printf("%-22s Vida: %2d/%2d  AtkF: %2d  AtkM: %2d  DefF: %2d  DefM: %2d\n",
           enemigo->name, vida(enemigo), statActual(enemigo, 0),
           statActual(enemigo, 1), statActual(enemigo, 2), statActual(enemigo, 3), statActual(enemigo, 4));
    printf("--------------------------------------------------\n");
}
 
// Si el defensor esta evadiendo, tira un volado: 50% esquiva el ataque.
// Regresa 1 si lo esquivo y 0 si el ataque le llega.
// La evasion dura todo el siguiente turno del oponente y se apaga al empezar el turno del defensor.
int intentaEsquivar(Personaje * atacante, Personaje * defensor){
    if(!defensor->evadir) return 0;
 
    if(rand() % 2 == 0){
        printf("%s esquiva lo que le avento %s!\n", defensor->name, atacante->name);
        return 1;
    }
 
    printf("%s intento esquivar, pero no le alcanzo!\n", defensor->name);
    return 0;
}
 
// Si el defensor esta evadiendo, tiene 50% de esquivarlo.
void ataqueFisico(Personaje * atacante, Personaje * defensor){
    printf("%s se lanza a golpear a %s. ", atacante->name, defensor->name);
    if(intentaEsquivar(atacante, defensor)) return;
 
    int danio = statActual(atacante, 1) - statActual(defensor, 3) / 2;
    if(danio < 1) danio = 1;
 
    defensor->danio += danio;
    printf("%s le mete un golpe a %s: %d de danio. Vida restante: %d\n",
           atacante->name, defensor->name, danio, vida(defensor));
}
 
// Las magias que se aplican a uno mismo no se pueden esquivar
int esMagiaOfensiva(FuncionMagia magia){
    return magia != electrolit && magia != attackbuff;
}
 
// Lanza la magia del personaje. Las ofensivas se pueden esquivar
void lanzarMagia(Personaje * lanza, Personaje * recibe, int n){
    FuncionMagia magia = *(lanza->magic + n);
 
    if(esMagiaOfensiva(magia) && recibe->evadir){
        printf("%s lanza %s. ", lanza->name, *(lanza->magic_name + n));
        if(intentaEsquivar(lanza, recibe)) return;
    }
 
    magia(lanza, recibe);   // Llamada a traves del apuntador a funcion
}
 
// Menu del turno del jugador
void turnoJugador(Personaje * jugador, Personaje * enemigo){
    int turno_usado = 0;
 
    while(!turno_usado){
        printf("\nTurno de %s. Que vas a hacer?\n", jugador->name);
        printf("\t1. Ataque fisico\n");
        printf("\t2. Magia\n");
        printf("\t3. Evadir el siguiente ataque\n");
        printf("\t4. Ver inventario\n");
 
        int opcion = pedirNumero(1, 4);
 
        if(opcion == 1){
            ataqueFisico(jugador, enemigo);
            turno_usado = 1;
        }else if(opcion == 2){
            printf("Tus magias:\n");
            for(int i = 0; i < 3; i++){
                printf("\t%d. %s\n", i + 1, *(jugador->magic_name + i));
            }
            int n = pedirNumero(1, 3) - 1;
            lanzarMagia(jugador, enemigo, n);
            turno_usado = 1;
        }else if(opcion == 3){
            jugador->evadir = 1;
            printf("%s se pone en guardia para el siguiente turno del rival (50%% de esquivar).\n", jugador->name);
            turno_usado = 1;
        }else{
            imprimirInventario(jugador);
        }
    }
}
 
// Turno del enemigo: 50% ataque fisico, 40% magia al azar, 10% evadir
void turnoCPU(Personaje * enemigo, Personaje * jugador){
    int accion = rand() % 10;
 
    printf("\nTurno de %s.\n", enemigo->name);
 
    if(accion < 5){
        ataqueFisico(enemigo, jugador);
    }else if(accion < 9){
        lanzarMagia(enemigo, jugador, rand() % 3);
    }else{
        enemigo->evadir = 1;
        printf("%s se pone en guardia para tu siguiente turno (50%% de esquivar).\n", enemigo->name);
    }
}
 
// Ciclo de batalla. Regresa 1 si gana el jugador y 0 si pierde
int batalla(Personaje * jugador, Personaje * enemigo){
    reiniciarEfectos(jugador);
    reiniciarEfectos(enemigo);
    int ganado = 1;
 
    mostrarEstado(jugador, enemigo);
 
    while(1){
        // ---- Turno del jugador ----
        jugador->evadir = 0;   // La evasion solo dura hasta su siguiente turno
        if(jugador->perder_Turno){
            printf("\n%s sigue sobandose las rodillas y pierde su turno.\n", jugador->name);
            jugador->perder_Turno = 0;
        }else{
            turnoJugador(jugador, enemigo);
        }
        avanzarEfectos(jugador);
 
        if(vida(enemigo) <= 0){
            ganado = 1;
            break;
        }
 
        // ---- Turno del enemigo ----
        enemigo->evadir = 0;
        if(enemigo->perder_Turno){
            printf("\n%s sigue sobandose las rodillas y pierde su turno.\n", enemigo->name);
            enemigo->perder_Turno = 0;
        }else{
            turnoCPU(enemigo, jugador);
        }
        avanzarEfectos(enemigo);
 
        if(vida(jugador) <= 0){
            ganado = 0;
            break;
        }
 
        mostrarEstado(jugador, enemigo);
    }
 
    return ganado;
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

    //Introduccion del lore
    introduccion(jugadores);

    for(int nivel = 1; nivel <= 4; nivel++){
        //Lore antes de cada batalla
        loreAntesDeBatalla(nivel, jugadores, jugadores + nivel);

        //Funcion batalla en la cual se dictamina si vencio o no el jugador
        int gano = batalla(jugadores, jugadores + nivel);
        if(!gano){
            //Lore de derrota
            loreDerrota(jugadores, jugadores + nivel);
            break;
        }
        //Lore de victoria
        loreVictoria(nivel, jugadores, jugadores + nivel);

        //Reclamo de recompensas
        if(nivel < 4){
            recompensas(jugadores, nivel);   // Después del jefe final ya no hay siguiente batalla
        }
    }

    return 0;
}

    