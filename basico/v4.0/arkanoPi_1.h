#ifndef _ARKANOPI_H_
#define _ARKANOPI_H_

#include <time.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <wiringPi.h>

#include "kbhit.h" // para poder detectar teclas pulsadas sin bloqueo y leer las teclas pulsadas
#include "fsm.h"
#include "arkanoPiLib.h"

#define CLK_MS 10

typedef enum {
	WAIT_START,
	WAIT_PUSH,
	WAIT_END} tipo_estados_juego;

typedef struct {
	tipo_arkanoPi arkanoPi;
	tipo_estados_juego estado;
	char teclaPulsada;
} tipo_juego;

//-----------------------------------------------------
//FUNCIONES DE ENTRADA O TRANSICION DE LA MAQUINA DE ESTADOS
//-----------------------------------------------------
//Prototipos de funciones de entrada
int CompruebaTeclaPulsada(fsm_t* this);
int CompruebaTeclaPelota(fsm_t* this);
int CompruebaTeclaRaquetaIzquierda(fsm_t* this);
int CompruebaTeclaRaquetaDerecha(fsm_t* this);
int CompruebaFinalJuego(fsm_t* this);

//----------------------------------------------------
//FUNCIONES DE SALIDA O DE ACCION DE LA MAQUINA DE ESTADOS
//----------------------------------------------------

//------------------------------------------------------
// FUNCIONES DE ACCION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------

void InicializaJuego (fsm_t*);
void MueveRaquetaIzquierda (fsm_t*);
void MueveRaquetaDerecha (fsm_t*);
void MovimientoPelota (fsm_t*);
void FinalJuego (fsm_t*);
void ReseteaJuego (fsm_t*);

//------------------------------------------------------
//FUNCIONES DE ATENCIÓN A LOS TEMPORIZADORES
//------------------------------------------------------
static void refresca_pelota(union sigval arg);
static void refresca_matriz(union sigval arg);

//------------------------------------------------------
// SUBRUTINAS DE ATENCION A LAS INTERRUPCIONES
//------------------------------------------------------
PI_THREAD (thread_explora_teclado);

//------------------------------------------------------
// FUNCIONES DE INICIALIZACION
//------------------------------------------------------
int systemSetup (void);

#endif /* ARKANOPI_H_ */
