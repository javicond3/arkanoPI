/** File Name          : arkanoPiLib.h
  * Description        : la instancia de métodos y estructuras de arkanoPiLib.c
  */
#ifndef _ARKANOPILIB_H_
#define _ARKANOPILIB_H_

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>

/* Defines -------------------------------------------------------------------*/
#define MATRIZ_ANCHO 	10
#define MATRIZ_ALTO 	7
#define LADRILLOS_ANCHO 10
#define LADRILLOS_ALTO 	2
#define RAQUETA_ANCHO 		3
#define RAQUETA_ALTO 		1

/* Structs -------------------------------------------------------------------*/
typedef struct {
	// Posicion
	int x;
	int y;
	// Forma
	int ancho;
	int alto;
} tipo_raqueta;

typedef struct {
	// Posicion
	int x;
	int y;
	// Trayectoria
	int xv;
	int yv;
} tipo_pelota;

typedef struct {
	// Matriz de ocupación de las distintas posiciones que conforman el display
	// (correspondiente al estado encendido/apagado de cada uno de los leds)
	int matriz[MATRIZ_ANCHO][MATRIZ_ALTO];
} tipo_pantalla;

typedef struct {
  tipo_pantalla ladrillos; // Notese que, por simplicidad, los ladrillos comparten tipo con la pantalla
  tipo_pantalla pantalla;
  tipo_raqueta raqueta;
  tipo_pelota pelota;
} tipo_arkanoPi;

typedef struct {
  tipo_pantalla pantalla;
  tipo_raqueta raqueta1;
  tipo_raqueta raqueta2;
  tipo_pelota pelota;
} tipo_arkanoPi_2p;

extern tipo_pantalla pantalla_inicial;

//------------------------------------------------------
// FUNCIONES DE INICIALIZACION / RESET
//------------------------------------------------------
void ReseteaMatriz(tipo_pantalla *p_pantalla);
void ReseteaLadrillos(tipo_pantalla *p_ladrillos, int masDisparos);
void ReseteaPelota(tipo_pelota *p_pelota);
void ReseteaRaqueta(tipo_raqueta *p_raqueta);
void ReseteaRaqueta_1(tipo_raqueta *p_raqueta);
void ReseteaRaqueta_2(tipo_raqueta *p_raqueta);

//------------------------------------------------------
// FUNCIONES DE VISUALIZACION (ACTUALIZACION DEL OBJETO PANTALLA QUE LUEGO USARA EL DISPLAY)
//------------------------------------------------------
void PintaMensajeInicialPantalla (tipo_pantalla *p_pantalla, tipo_pantalla *p_pantalla_inicial);
void PintaPantallaPorTerminal (tipo_pantalla *p_pantalla);
void PintaLadrillos(tipo_pantalla *p_ladrillos, tipo_pantalla *p_pantalla);
void PintaRaqueta(tipo_raqueta *p_raqueta, tipo_pantalla *p_pantalla);
void PintaPelota(tipo_pelota *p_pelota, tipo_pantalla *p_pantalla);
void ActualizaPantalla(tipo_arkanoPi* p_arkanoPi);
void ActualizaPantalla_2(tipo_arkanoPi_2p* p_arkanoPi);

void InicializaArkanoPi(tipo_arkanoPi *p_arkanoPi, int masDisparos);
void InicializaArkanoPi_2(tipo_arkanoPi_2p *p_arkanoPi);
int CalculaLadrillosRestantes(tipo_pantalla *p_ladrillos);

#endif /* _ARKANOPILIB_H_ */
