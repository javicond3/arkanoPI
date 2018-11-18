/** File Name          : arkanoPi_1.c
  * Description        : Main program body
  */

/* Includes ------------------------------------------------------------------*/
#include "arkanoPi_1.h"
#include "kbhit.h" //Para detectar teclas pulsadas sin bloqueo y leer las teclas pulsadas
#include "fsm.h" //Para poder crear y ejecutar la maquina de estados
#include "time.h"
#include "tmr.h"
#include "wiringPi.h"
#include "wiringPiSPI.h"


/* Defines -------------------------------------------------------------------*/
#define CLK_MS 10 // PERIODO DE ACTUALIZACION DE LA MAQUINA DE ESTADOS

// FLAGS DEL SISTEMA
#define FLAG_TECLA 0x01
#define FLAG_PELOTA 0x02
#define FLAG_RAQUETA_DERECHA 0x04
#define FLAG_RAQUETA_IZQUIERDA 0x08
#define FLAG_FINAL_JUEGO 0x10
#define FLAG_JOYSTICK 0x20
#define FLAG_PAUSE 0x40
#define FLAG_SUBMENU 0x80

#define PIN_BOTON_DCHA 16
#define PIN_BOTON_IZDA 19
#define PIN_BOTON_SUBMENU 20
#define DEBOUNCE_TIME 200
#define PAUSE_TIME 60

#define SPI_ADC_CH 0
#define SPI_ADC_FREQ 1000000

volatile int flags = 0;

//mutex para los 2 procesos
#define FLAGS_KEY 1
#define STD_IO_BUFFER_KEY 2

static volatile tipo_juego juego;
static volatile tipo_juego_2 juego2;

tipo_pantalla inicial;
static tmr_t* tmr;
static tmr_t* tmr_pelota;
static tmr_t* tmr_joystick;

/* Private variables ---------------------------------------------------------*/
int juegoActual = 0; // 0 === ArkanoPi / 1 === Pong
int columna = 0;
int debounceTime;
int pausaDerecha = 0;
int pausaIzquierda = 0;
int vidas1 = 3;
int vidas2 = 3;
int masDisparos = 1; //Si es 1 tengo que darle dos veces a un ladrillo para derribarlo
int submenu = 0; //Para desplazarme en el submenu

//------------------------------------------------------
//FUNCIONES DE LA MAQUINA DE ESTADOS
//------------------------------------------------------

/**
 * @brief			Función que introduce un retardo
 * @param	next	Entero que indica cuanto es el retardo
 * @return			Devuelve el resultado calculado de a x privateVariable
 */

void delay_until(unsigned int next) {
	unsigned int now = millis();

	if (next > now) {
		delay(next - now);
	}
}

/**
 * @brief			Función que comprueba si se activó el flag correspondiente
 * @param	this	Máquina de estados
 * @return			Entero que indica si se pusló la tecla(o pulsador)
 */
int CompruebaTeclaPulsada(fsm_t* this) {
	int result;

	piLock(FLAGS_KEY);
	result = (flags & FLAG_TECLA);
	piUnlock(FLAGS_KEY);

	return result;
}

/**
 * @brief			Función que comprueba si se activó el flag correspondiente
 * @param	this	Máquina de estados
 * @return			Entero que indica si se pusló la tecla(o pulsador)
 */
int CompruebaTeclaPelota(fsm_t* this) {
	int result;

	piLock(FLAGS_KEY);
	result = (flags & FLAG_PELOTA);
	piUnlock(FLAGS_KEY);

	return result;
}

/**
 * @brief			Función que comprueba si se activó el flag correspondiente
 * @param	this	Máquina de estados
 * @return			Entero que indica si se pusló la tecla(o pulsador)
 */
int CompruebaTeclaRaquetaIzquierda(fsm_t* this) {
	int result;

	piLock(FLAGS_KEY);
	result = (flags & FLAG_RAQUETA_IZQUIERDA);
	piUnlock(FLAGS_KEY);

	return result;
}

/**
 * @brief			Función que comprueba si se activó el flag correspondiente
 * @param	this	Máquina de estados
 * @return			Entero que indica si se pusló la tecla(o pulsador)
 */
int CompruebaTeclaRaquetaDerecha(fsm_t* this) {
	int result;

	piLock(FLAGS_KEY);
	result = (flags & FLAG_RAQUETA_DERECHA);
	piUnlock(FLAGS_KEY);

	return result;
}

/**
 * @brief			Función que comprueba si se activó el flag correspondiente
 * @param	this	Máquina de estados
 * @return			Entero que indica si se pusló la tecla(o pulsador)
 */
int CompruebaFinalJuego(fsm_t* this) {
	int result;

	piLock(FLAGS_KEY);
	result = (flags & FLAG_FINAL_JUEGO);
	piUnlock(FLAGS_KEY);

	return result;
}

/**
 * @brief			Función que comprueba si se activó el flag correspondiente
 * @param	this	Máquina de estados
 * @return			Entero que indica si se pusló la tecla(o pulsador)
 */
int CompruebaJoystick(fsm_t* this) {
	int result;

	piLock(FLAGS_KEY);
	result = (flags & FLAG_JOYSTICK);
	piUnlock(FLAGS_KEY);

	return result;
}

/**
 * @brief			Función que comprueba si se activó el flag correspondiente
 * @param	this	Máquina de estados
 * @return			Entero que indica si se pusló la tecla(o pulsador)
 */
int CompruebaPause(fsm_t* this) {
	int result;

	piLock(FLAGS_KEY);
	result = (flags & FLAG_PAUSE);
	piUnlock(FLAGS_KEY);

	return result;
}
/**
 * @brief			Función que comprueba si se activó el flag correspondiente
 * @param	this	Máquina de estados
 * @return			Entero que indica si se pusló la tecla(o pulsador)
 */
int CompruebaTeclaSubmenu(fsm_t* this) {
	int result;

	piLock(FLAGS_KEY);
	result = (flags & FLAG_SUBMENU);
	piUnlock(FLAGS_KEY);

	return result;
}

//------------------------------------------------------
// FUNCIONES DE ACCION
//------------------------------------------------------

/**
 * @brief			Te pinta el menú de selección de juego
 * @param	this		Máquina de estados
 */
void abreMenu(fsm_t* fsm) {
	piLock(FLAGS_KEY);
	flags &= ~FLAG_TECLA;
	flags &= ~FLAG_PELOTA;
	flags &= ~FLAG_RAQUETA_DERECHA;
	flags &= ~FLAG_RAQUETA_IZQUIERDA;
	flags &= ~FLAG_FINAL_JUEGO;
	flags &= ~FLAG_JOYSTICK;
	flags &= ~FLAG_PAUSE;
	piUnlock(FLAGS_KEY);

	piLock(STD_IO_BUFFER_KEY);
	//Matriz que pinta 1 y 2 para elegir el juego 1 === ArkanoPi 2===Pong
	int matrizMenu[MATRIZ_ANCHO][MATRIZ_ALTO] = { { 0, 0, 0, 0, 0, 0, 0 }, { 0,
			0, 1, 0, 0, 1, 0 }, { 0, 1, 1, 1, 1, 1, 0 },
			{ 0, 0, 0, 0, 0, 1, 0 }, { 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0,
					0, 0 }, { 0, 1, 0, 1, 1, 1, 0 }, { 0, 1, 0, 1, 0, 1, 0 }, {
					0, 1, 1, 1, 0, 1, 0 }, { 0, 0, 0, 0, 0, 0, 0 }, };

	int i, j = 0;

	for (i = 0; i < MATRIZ_ANCHO; i++) {
		for (j = 0; j < MATRIZ_ALTO; j++) {
			inicial.matriz[i][j] = matrizMenu[i][j];
		}
	}
	if (juegoActual == 0) {
		PintaMensajeInicialPantalla((tipo_pantalla*) &juego.arkanoPi.pantalla,
				(tipo_pantalla*) &inicial);
		PintaPantallaPorTerminal((tipo_pantalla*) &juego.arkanoPi.pantalla);
	} else if (juegoActual == 1) {
		PintaMensajeInicialPantalla(
				(tipo_pantalla*) &juego2.arkanoPi_2p.pantalla,
				(tipo_pantalla*) &inicial);
		PintaPantallaPorTerminal((tipo_pantalla*) &juego2.arkanoPi_2p.pantalla);
	}

	piUnlock(STD_IO_BUFFER_KEY);
}

/**
 * @brief			Te pinta el menú de configuración de juego
 * @param	this	Máquina de estados
 * @note			Usa la variable global submenu para desplazarse por el en switch
 */
void abreSubmenu1(fsm_t* fsm) {
	piLock(FLAGS_KEY);
	flags &= ~FLAG_TECLA;
	flags &= ~FLAG_PELOTA;
	flags &= ~FLAG_RAQUETA_DERECHA;
	flags &= ~FLAG_SUBMENU;
	flags &= ~FLAG_RAQUETA_IZQUIERDA;
	flags &= ~FLAG_FINAL_JUEGO;
	flags &= ~FLAG_JOYSTICK;
	flags &= ~FLAG_PAUSE;
	piUnlock(FLAGS_KEY);
	if (millis() < debounceTime) {
		debounceTime = millis() + DEBOUNCE_TIME;
		return;
	}
	//Matriz que pinta 1 y 2 para elegir el nº de vidas 1 ===0 Vidas 2=== 3 vidas
	int matrizSubMenu0[MATRIZ_ANCHO][MATRIZ_ALTO] = { { 1, 1, 1, 1, 1, 1, 1 }, {
			0, 0, 0, 0, 0, 0, 1 }, { 0, 0, 0, 0, 0, 0, 1 }, { 0, 0, 0, 0, 0, 0,
			0 }, { 1, 1, 1, 1, 1, 1, 1 }, { 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0,
			0, 0, 0 }, { 1, 1, 1, 1, 1, 1, 1 }, { 0, 0, 0, 0, 0, 0, 0 }, { 1, 1,
			1, 1, 1, 1, 1 }, };
	//Matriz que pinta 1 y 2 para elegir el nº de golpes para derribar ladrillo
	//1 ===1 golpe 2=== 2 golpes
	int matrizSubMenu1[MATRIZ_ANCHO][MATRIZ_ALTO] = { { 1, 1, 1, 1, 1, 1, 1 }, {
			1, 0, 0, 1, 0, 0, 1 }, { 0, 1, 1, 0, 1, 1, 0 }, { 0, 0, 0, 0, 0, 0,
			0 }, { 1, 1, 1, 1, 1, 1, 1 }, { 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0,
			0, 0, 0 }, { 1, 1, 1, 1, 1, 1, 1 }, { 0, 0, 0, 0, 0, 0, 0 }, { 1, 1,
			1, 1, 1, 1, 1 }, };
	//Matriz que pinta OK para indicar que ya seleccionó el menú
	int matrizSubMenu2[MATRIZ_ANCHO][MATRIZ_ALTO] = { { 1, 1, 1, 1, 1, 1, 1 }, {
			1, 0, 0, 0, 0, 0, 1 }, { 1, 0, 0, 0, 0, 0, 1 }, { 1, 0, 0, 0, 0, 0,
			1 }, { 1, 1, 1, 1, 1, 1, 1 }, { 0, 0, 0, 0, 0, 0, 0 }, { 1, 1, 1, 1,
			1, 1, 1 }, { 0, 0, 1, 0, 1, 0, 0 }, { 0, 1, 0, 0, 0, 1, 0 }, { 1, 0,
			0, 0, 0, 0, 1 }, };

	piLock(STD_IO_BUFFER_KEY);
	int i, j = 0;
	switch (submenu) {
	case 0:
		for (i = 0; i < MATRIZ_ANCHO; i++) {
			for (j = 0; j < MATRIZ_ALTO; j++) {
				inicial.matriz[i][j] = matrizSubMenu0[i][j];
			}
		}
		break;
	case 1:
		for (i = 0; i < MATRIZ_ANCHO; i++) {
			for (j = 0; j < MATRIZ_ALTO; j++) {
				inicial.matriz[i][j] = matrizSubMenu1[i][j];
			}
		}
		vidas1 = 0;
		break;
	case 2:

		for (i = 0; i < MATRIZ_ANCHO; i++) {
			for (j = 0; j < MATRIZ_ALTO; j++) {
				inicial.matriz[i][j] = matrizSubMenu2[i][j];
			}
		}
		masDisparos = 0;
		break;
	default:

		for (i = 0; i < MATRIZ_ANCHO; i++) {
			for (j = 0; j < MATRIZ_ALTO; j++) {
				inicial.matriz[i][j] = matrizSubMenu2[i][j];
			}
		}
		break;

	}

	PintaMensajeInicialPantalla((tipo_pantalla*) &juego.arkanoPi.pantalla,
			(tipo_pantalla*) &inicial);
	PintaPantallaPorTerminal((tipo_pantalla*) &juego.arkanoPi.pantalla);
	submenu++;

	piUnlock(STD_IO_BUFFER_KEY);
}

/**
 * @brief			Te pinta el menú de configuración de juego
 * @param	this	Máquina de estados
 * @note			Usa la variable global submenu para desplazarse por el en switch
 */
void abreSubmenu2(fsm_t* fsm) {
	piLock(FLAGS_KEY);
	flags &= ~FLAG_TECLA;
	flags &= ~FLAG_PELOTA;
	flags &= ~FLAG_RAQUETA_DERECHA;
	flags &= ~FLAG_SUBMENU;
	flags &= ~FLAG_RAQUETA_IZQUIERDA;
	flags &= ~FLAG_FINAL_JUEGO;
	flags &= ~FLAG_JOYSTICK;
	flags &= ~FLAG_PAUSE;
	piUnlock(FLAGS_KEY);
	if (millis() < debounceTime) {
		debounceTime = millis() + DEBOUNCE_TIME;
		return;
	}
	//Matriz que pinta 1 y 2 para elegir el nº de vidas 1 ===0 Vidas 2=== 3 vidas
	int matrizSubMenu0[MATRIZ_ANCHO][MATRIZ_ALTO] = { { 1, 1, 1, 1, 1, 1, 1 }, {
			0, 0, 0, 0, 0, 0, 1 }, { 0, 0, 0, 0, 0, 0, 1 }, { 0, 0, 0, 0, 0, 0,
			0 }, { 1, 1, 1, 1, 1, 1, 1 }, { 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0,
			0, 0, 0 }, { 1, 1, 1, 1, 1, 1, 1 }, { 0, 0, 0, 0, 0, 0, 0 }, { 1, 1,
			1, 1, 1, 1, 1 }, };
	//Matriz que pinta 1 y 2 para elegir el nº de golpes para derribar ladrillo
		//1 ===1 golpe 2=== 2 golpes
	int matrizSubMenu1[MATRIZ_ANCHO][MATRIZ_ALTO] = { { 1, 1, 1, 1, 1, 1, 1 }, {
			1, 0, 0, 1, 0, 0, 1 }, { 0, 1, 1, 0, 1, 1, 0 }, { 0, 0, 0, 0, 0, 0,
			0 }, { 1, 1, 1, 1, 1, 1, 1 }, { 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0,
			0, 0, 0 }, { 1, 1, 1, 1, 1, 1, 1 }, { 0, 0, 0, 0, 0, 0, 0 }, { 1, 1,
			1, 1, 1, 1, 1 }, };
	//Matriz que pinta OK para indicar que ya seleccionó el menú
	int matrizSubMenu2[MATRIZ_ANCHO][MATRIZ_ALTO] = { { 1, 1, 1, 1, 1, 1, 1 }, {
			1, 0, 0, 0, 0, 0, 1 }, { 1, 0, 0, 0, 0, 0, 1 }, { 1, 0, 0, 0, 0, 0,
			1 }, { 1, 1, 1, 1, 1, 1, 1 }, { 0, 0, 0, 0, 0, 0, 0 }, { 1, 1, 1, 1,
			1, 1, 1 }, { 0, 0, 1, 0, 1, 0, 0 }, { 0, 1, 0, 0, 0, 1, 0 }, { 1, 0,
			0, 0, 0, 0, 1 }, };

	piLock(STD_IO_BUFFER_KEY);
	int i, j = 0;
	switch (submenu) {
	case 0:
		for (i = 0; i < MATRIZ_ANCHO; i++) {
			for (j = 0; j < MATRIZ_ALTO; j++) {
				inicial.matriz[i][j] = matrizSubMenu0[i][j];
			}
		}
		break;
	case 1:
		for (i = 0; i < MATRIZ_ANCHO; i++) {
			for (j = 0; j < MATRIZ_ALTO; j++) {
				inicial.matriz[i][j] = matrizSubMenu1[i][j];
			}
		}
		vidas1 = 3;
		break;
	case 2:

		for (i = 0; i < MATRIZ_ANCHO; i++) {
			for (j = 0; j < MATRIZ_ALTO; j++) {
				inicial.matriz[i][j] = matrizSubMenu2[i][j];
			}
		}
		masDisparos = 1;
		break;
	default:

		for (i = 0; i < MATRIZ_ANCHO; i++) {
			for (j = 0; j < MATRIZ_ALTO; j++) {
				inicial.matriz[i][j] = matrizSubMenu2[i][j];
			}
		}
		break;

	}

	PintaMensajeInicialPantalla((tipo_pantalla*) &juego.arkanoPi.pantalla,
			(tipo_pantalla*) &inicial);
	PintaPantallaPorTerminal((tipo_pantalla*) &juego.arkanoPi.pantalla);
	submenu++;

	piUnlock(STD_IO_BUFFER_KEY);
}

/**
 * @brief			funcion encargada de inicializar  toda variable o estructura de datos.
 * @param	fsm		Maquina de estados
 */
void InicializaJuego(fsm_t* fsm) {
	piLock(FLAGS_KEY);
	flags &= ~FLAG_TECLA;
	flags &= ~FLAG_PELOTA;
	flags &= ~FLAG_RAQUETA_DERECHA;
	flags &= ~FLAG_RAQUETA_IZQUIERDA;
	flags &= ~FLAG_FINAL_JUEGO;
	flags &= ~FLAG_JOYSTICK;
	flags &= ~FLAG_PAUSE;
	piUnlock(FLAGS_KEY);

	piLock(STD_IO_BUFFER_KEY);
	InicializaArkanoPi((tipo_arkanoPi*) &juego.arkanoPi, masDisparos);
	ActualizaPantalla((tipo_arkanoPi*) &juego.arkanoPi);
	PintaPantallaPorTerminal((tipo_pantalla*) &juego.arkanoPi.pantalla);
	printf("INICIO ARKANOPI \n");
	piUnlock(STD_IO_BUFFER_KEY);

	juegoActual = 0; //Elegimos el arkanoPi
	submenu = 0; //Reinicio el offset del submenu para el próximo arranque
	tmr_pelota = tmr_new(refresca_pelota);
	tmr_startms(tmr_pelota, 1500);

	tmr_joystick = tmr_new(refresca_joystick);
	tmr_startms(tmr_joystick, 100);
}

void InicializaJuego_2(fsm_t* fsm) {
	piLock(FLAGS_KEY);
	flags &= ~FLAG_TECLA;
	flags &= ~FLAG_PELOTA;
	flags &= ~FLAG_RAQUETA_DERECHA;
	flags &= ~FLAG_RAQUETA_IZQUIERDA;
	flags &= ~FLAG_FINAL_JUEGO;
	flags &= ~FLAG_JOYSTICK;
	flags &= ~FLAG_PAUSE;
	piUnlock(FLAGS_KEY);

	piLock(STD_IO_BUFFER_KEY);

	//PINTAR PANTALLA E INICIALIZAR PONG
	InicializaArkanoPi_2((tipo_arkanoPi_2p*) &juego2.arkanoPi_2p);
	ActualizaPantalla_2((tipo_arkanoPi_2p*) &juego2.arkanoPi_2p);
	PintaPantallaPorTerminal((tipo_pantalla*) &juego2.arkanoPi_2p.pantalla);
	printf("INICIO PONG \n");
	piUnlock(STD_IO_BUFFER_KEY);

	juegoActual = 1; //Elegimos el Pong
	vidas1 = 3; //Ponemos las vidas
	vidas2 = 3; //Ponemos las vidas
	tmr_pelota = tmr_new(refresca_pelota);
	tmr_startms(tmr_pelota, 1500);

	tmr_joystick = tmr_new(refresca_joystick);
	tmr_startms(tmr_joystick, 100);
}

/**
 * @brief			funcion encargada de mover la raqueta a la izquierda.
 * @param	fsm		Maquina de estados
 */
void MueveRaquetaIzquierda(fsm_t* fsm) {
	piLock(FLAGS_KEY);
	flags &= ~FLAG_TECLA;
	flags &= ~FLAG_PELOTA;
	flags &= ~FLAG_RAQUETA_DERECHA;
	flags &= ~FLAG_RAQUETA_IZQUIERDA;
	flags &= ~FLAG_FINAL_JUEGO;
	flags &= ~FLAG_JOYSTICK;
	flags &= ~FLAG_PAUSE;
	piUnlock(FLAGS_KEY);

	if (juegoActual == 0) {
		if (millis() < debounceTime) {
			debounceTime = millis() + DEBOUNCE_TIME;
			return;
		}

		piLock(STD_IO_BUFFER_KEY);
		if (juego.arkanoPi.raqueta.x > -2)
			juego.arkanoPi.raqueta.x -= 1;
		ActualizaPantalla((tipo_arkanoPi*) &juego.arkanoPi);
		PintaPantallaPorTerminal((tipo_pantalla*) &juego.arkanoPi.pantalla);
		piUnlock(STD_IO_BUFFER_KEY);

		while (digitalRead(PIN_BOTON_DCHA) == HIGH) {
			delay(1);
		}
		debounceTime = millis() + DEBOUNCE_TIME;
	} else if (juegoActual == 1) {
		if (millis() < debounceTime) {
			debounceTime = millis() + DEBOUNCE_TIME;
			return;
		}

		piLock(STD_IO_BUFFER_KEY);
		if (juego2.arkanoPi_2p.raqueta2.x > -2)
			juego2.arkanoPi_2p.raqueta2.x -= 1;
		ActualizaPantalla_2((tipo_arkanoPi_2p*) &juego2.arkanoPi_2p);
		PintaPantallaPorTerminal((tipo_pantalla*) &juego2.arkanoPi_2p.pantalla);
		piUnlock(STD_IO_BUFFER_KEY);

		while (digitalRead(PIN_BOTON_IZDA) == HIGH) {
			delay(1);
		}
		debounceTime = millis() + DEBOUNCE_TIME;
	} else {
		printf("ERROR!! NO HAY JUEGO :( \n");
	}

}

/**
 * @brief			funcion encargada de mover la raqueta a la derecha.
 * @param	fsm		Maquina de estados
 */
void MueveRaquetaDerecha(fsm_t* fsm) {
	piLock(FLAGS_KEY);
	flags &= ~FLAG_TECLA;
	flags &= ~FLAG_PELOTA;
	flags &= ~FLAG_RAQUETA_DERECHA;
	flags &= ~FLAG_RAQUETA_IZQUIERDA;
	flags &= ~FLAG_FINAL_JUEGO;
	flags &= ~FLAG_JOYSTICK;
	flags &= ~FLAG_PAUSE;
	piUnlock(FLAGS_KEY);

	if (juegoActual == 0) {
		if (millis() < debounceTime) {
			debounceTime = millis() + DEBOUNCE_TIME;
			return;
		}

		piLock(STD_IO_BUFFER_KEY);
		if (juego.arkanoPi.raqueta.x < 9)
			juego.arkanoPi.raqueta.x += 1;
		ActualizaPantalla((tipo_arkanoPi*) &juego.arkanoPi);
		PintaPantallaPorTerminal((tipo_pantalla*) &juego.arkanoPi.pantalla);
		piUnlock(STD_IO_BUFFER_KEY);

		while (digitalRead(PIN_BOTON_DCHA) == HIGH) {
			delay(1);
		}
		debounceTime = millis() + DEBOUNCE_TIME;
	} else if (juegoActual == 1) {
		if (millis() < debounceTime) {
			debounceTime = millis() + DEBOUNCE_TIME;
			return;
		}

		piLock(STD_IO_BUFFER_KEY);
		if (juego2.arkanoPi_2p.raqueta2.x < 9)
			juego2.arkanoPi_2p.raqueta2.x += 1;
		ActualizaPantalla_2((tipo_arkanoPi_2p*) &juego2.arkanoPi_2p);
		PintaPantallaPorTerminal((tipo_pantalla*) &juego2.arkanoPi_2p.pantalla);
		piUnlock(STD_IO_BUFFER_KEY);

		while (digitalRead(PIN_BOTON_DCHA) == HIGH) {
			delay(1);
		}
		debounceTime = millis() + DEBOUNCE_TIME;
	} else {

	}

}

/**
 * @brief			funcion encargada de actualizar la posición de la pelota.
 * @param	fsm		Maquina de estados
 * @note			Usa la variable global juego para saber si el juego en el que estamos.
 */
void MovimientoPelota(fsm_t* fsm) {

	piLock(FLAGS_KEY);
	flags &= ~FLAG_TECLA;
	flags &= ~FLAG_PELOTA;
	flags &= ~FLAG_RAQUETA_DERECHA;
	flags &= ~FLAG_RAQUETA_IZQUIERDA;
	flags &= ~FLAG_FINAL_JUEGO;
	flags &= ~FLAG_JOYSTICK;
	flags &= ~FLAG_PAUSE;
	piUnlock(FLAGS_KEY);

	if (juegoActual == 0) {
		piLock(STD_IO_BUFFER_KEY);
		int xHipotetica = juego.arkanoPi.pelota.x + juego.arkanoPi.pelota.xv;
		int yHipotetica = juego.arkanoPi.pelota.y + juego.arkanoPi.pelota.yv;
		int rebotoX = 0; //Indica si reboto o no con las paredes
		int rebotoY = 0; //Indica si reboto o no con el suelo o techo
		int fin = 0;

		//Rebote pared
		if (xHipotetica == -1 || xHipotetica == 10) {
			juego.arkanoPi.pelota.xv = -juego.arkanoPi.pelota.xv;
			juego.arkanoPi.pelota.x += juego.arkanoPi.pelota.xv;
			xHipotetica = juego.arkanoPi.pelota.x;
			rebotoX = 1;
		}
		//Rebote techo o ladrillo
		if (yHipotetica == -1
				|| juego.arkanoPi.ladrillos.matriz[xHipotetica][yHipotetica]
						>= 1) {
			if (juego.arkanoPi.ladrillos.matriz[xHipotetica][yHipotetica] >= 1)
				juego.arkanoPi.ladrillos.matriz[xHipotetica][yHipotetica] -= 1;
			juego.arkanoPi.pelota.yv = -juego.arkanoPi.pelota.yv;
			juego.arkanoPi.pelota.y += juego.arkanoPi.pelota.yv;
			yHipotetica = juego.arkanoPi.pelota.y;
			rebotoY = 1;
			if (CalculaLadrillosRestantes(
					(tipo_pantalla*) &juego.arkanoPi.ladrillos) == 0)
				fin = 1;
		}
		//Rebote suelo o pala
		if (yHipotetica == 6) {
			int posRaqueta = juego.arkanoPi.raqueta.x;
			if (xHipotetica >= posRaqueta
					&& xHipotetica
							< (posRaqueta + juego.arkanoPi.raqueta.ancho)) {
				switch (xHipotetica - posRaqueta) {
				case 0:
					juego.arkanoPi.pelota.yv = -1;
					juego.arkanoPi.pelota.xv = -1;
					break;
				case 1:
					juego.arkanoPi.pelota.yv = -1;
					juego.arkanoPi.pelota.xv = 0;
					break;
				case 2:
					juego.arkanoPi.pelota.yv = -1;
					juego.arkanoPi.pelota.xv = 1;
					break;
				}
				juego.arkanoPi.pelota.y += juego.arkanoPi.pelota.yv;
				juego.arkanoPi.pelota.x += juego.arkanoPi.pelota.xv;
				rebotoY = 1;
				rebotoX = 1;

			} else {
				fin = 1;
				juego.arkanoPi.pelota.y += juego.arkanoPi.pelota.yv;
				juego.arkanoPi.pelota.x += juego.arkanoPi.pelota.xv;
				rebotoY = 1;
				rebotoX = 1;
			}
		}
		if (!rebotoY)
			juego.arkanoPi.pelota.y += juego.arkanoPi.pelota.yv;
		if (!rebotoX)
			juego.arkanoPi.pelota.x += juego.arkanoPi.pelota.xv;

		ActualizaPantalla((tipo_arkanoPi*) &juego.arkanoPi);
		PintaPantallaPorTerminal((tipo_pantalla*) &juego.arkanoPi.pantalla);
		if (fin == 1) {
			vidas1--;
			if (vidas1 <= 0
					|| CalculaLadrillosRestantes(
							(tipo_pantalla*) &juego.arkanoPi.ladrillos) == 0) {
				piLock(FLAGS_KEY);
				flags |= FLAG_FINAL_JUEGO;
				piUnlock(FLAGS_KEY);
			} else {
				juego.arkanoPi.pelota.x = 4;
				juego.arkanoPi.pelota.y = 2;
				juego.arkanoPi.pelota.xv = 0;
				juego.arkanoPi.pelota.yv = 1;
			}

		}
		piUnlock(STD_IO_BUFFER_KEY);

		//MOVIMIENTO DEL PONG
	} else if (juegoActual == 1) {
		piLock(STD_IO_BUFFER_KEY);

		int xHipotetica = juego2.arkanoPi_2p.pelota.x
				+ juego2.arkanoPi_2p.pelota.xv;
		int yHipotetica = juego2.arkanoPi_2p.pelota.y
				+ juego2.arkanoPi_2p.pelota.yv;
		int posRaqueta1 = juego2.arkanoPi_2p.raqueta1.x;
		int posRaqueta2 = juego2.arkanoPi_2p.raqueta2.x;
		int rebotoX = 0; //Indica si reboto o no con las paredes
		int rebotoY = 0; //Indica si reboto o no con el suelo o techo
		int fin = 0;

		//Rebote pared
		if (xHipotetica == -1 || xHipotetica == 10) {
			juego2.arkanoPi_2p.pelota.xv = -juego2.arkanoPi_2p.pelota.xv;
			juego2.arkanoPi_2p.pelota.x += juego2.arkanoPi_2p.pelota.xv;
			xHipotetica = juego2.arkanoPi_2p.pelota.x;
			rebotoX = 1;
		}
		//Rebote palas o suelo
		if (yHipotetica == 6) {
			if (xHipotetica >= posRaqueta1
					&& xHipotetica
							< (posRaqueta1 + juego2.arkanoPi_2p.raqueta1.ancho)) {
				switch (xHipotetica - posRaqueta1) {
				case 0:
					juego2.arkanoPi_2p.pelota.yv = -1;
					juego2.arkanoPi_2p.pelota.xv = -1;
					break;
				case 1:
					juego2.arkanoPi_2p.pelota.yv = -1;
					juego2.arkanoPi_2p.pelota.xv = 0;
					break;
				case 2:
					juego2.arkanoPi_2p.pelota.yv = -1;
					juego2.arkanoPi_2p.pelota.xv = 1;
					break;
				}
				juego2.arkanoPi_2p.pelota.y += juego2.arkanoPi_2p.pelota.yv;
				juego2.arkanoPi_2p.pelota.x += juego2.arkanoPi_2p.pelota.xv;
				rebotoY = 1;
				rebotoX = 1;
			} else {
				juego2.arkanoPi_2p.pelota.y += juego2.arkanoPi_2p.pelota.yv;
				juego2.arkanoPi_2p.pelota.x += juego2.arkanoPi_2p.pelota.xv;
				fin = 1;
				rebotoY = 1;
				rebotoX = 1;
			}

		}
		if (yHipotetica == 0) {
			if (xHipotetica >= posRaqueta2
					&& xHipotetica
							< (posRaqueta2 + juego2.arkanoPi_2p.raqueta2.ancho)) {
				switch (xHipotetica - posRaqueta2) {
				case 0:
					juego2.arkanoPi_2p.pelota.yv = 1;
					juego2.arkanoPi_2p.pelota.xv = -1;
					break;
				case 1:
					juego2.arkanoPi_2p.pelota.yv = 1;
					juego2.arkanoPi_2p.pelota.xv = 0;
					break;
				case 2:
					juego2.arkanoPi_2p.pelota.yv = 1;
					juego2.arkanoPi_2p.pelota.xv = 1;
					break;
				}
				juego2.arkanoPi_2p.pelota.y += juego2.arkanoPi_2p.pelota.yv;
				juego2.arkanoPi_2p.pelota.x += juego2.arkanoPi_2p.pelota.xv;
				rebotoY = 1;
				rebotoX = 1;
			} else {
				juego2.arkanoPi_2p.pelota.y += juego2.arkanoPi_2p.pelota.yv;
				juego2.arkanoPi_2p.pelota.x += juego2.arkanoPi_2p.pelota.xv;
				fin = 1;
				rebotoY = 1;
				rebotoX = 1;
			}

		}
		if (!rebotoY)
			juego2.arkanoPi_2p.pelota.y += juego2.arkanoPi_2p.pelota.yv;
		if (!rebotoX)
			juego2.arkanoPi_2p.pelota.x += juego2.arkanoPi_2p.pelota.xv;

		ActualizaPantalla_2((tipo_arkanoPi_2p*) &juego2.arkanoPi_2p);
		PintaPantallaPorTerminal((tipo_pantalla*) &juego2.arkanoPi_2p.pantalla);
		if (fin == 1) {
			if (juego2.arkanoPi_2p.pelota.y == 6) {
				vidas1--;
			}
			if (juego2.arkanoPi_2p.pelota.y == 0) {
				vidas2--;
			}
			if (vidas1 == 0 || vidas2 == 0) {
				//Pintar pantalla fin de juego
				piLock(FLAGS_KEY);
				flags |= FLAG_FINAL_JUEGO;
				piUnlock(FLAGS_KEY);
			} else {
				//Vuelvo a empezar la partida quitandole una vida al que fallo
				juego2.arkanoPi_2p.pelota.x = 4;
				juego2.arkanoPi_2p.pelota.y = 2;
				juego2.arkanoPi_2p.pelota.xv = 0;
				juego2.arkanoPi_2p.pelota.yv = 1;
			}

		}
		piUnlock(STD_IO_BUFFER_KEY);
	} else {
		printf("ERROR!! NO HAY JUEGO :( \n");
	}

}

/**
 * @brief			funcion encargada de mostrar  los mensajes necesarios para informar acerca del resultado.
 * @param	fsm		Maquina de estados
 */

void FinalJuego(fsm_t* fsm) {
	piLock(FLAGS_KEY);
	flags &= ~FLAG_TECLA;
	flags &= ~FLAG_PELOTA;
	flags &= ~FLAG_RAQUETA_DERECHA;
	flags &= ~FLAG_RAQUETA_IZQUIERDA;
	flags &= ~FLAG_FINAL_JUEGO;
	flags &= ~FLAG_JOYSTICK;
	flags &= ~FLAG_PAUSE;
	piUnlock(FLAGS_KEY);

	if (juegoActual == 0) {
		piLock(STD_IO_BUFFER_KEY);
		int ladrillosQuedan = CalculaLadrillosRestantes(
				(tipo_pantalla*) &juego.arkanoPi.ladrillos);
		if (ladrillosQuedan != 0) {
			int matrizPerder[MATRIZ_ANCHO][MATRIZ_ALTO] = { { 0, 0, 0, 0, 0, 0,
					0 }, { 0, 1, 1, 0, 0, 0, 0 }, { 0, 1, 1, 0, 0, 1, 0 }, { 0,
					0, 0, 0, 1, 0, 0 }, { 0, 0, 0, 0, 1, 0, 0 }, { 0, 0, 0, 0,
					1, 0, 0 }, { 0, 0, 0, 0, 1, 0, 0 }, { 0, 1, 1, 0, 0, 1, 0 },
					{ 0, 1, 1, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0 }, };

			int i, j = 0;

			for (i = 0; i < MATRIZ_ANCHO; i++) {
				for (j = 0; j < MATRIZ_ALTO; j++) {
					inicial.matriz[i][j] = matrizPerder[i][j];
				}
			}
			PintaMensajeInicialPantalla(
					(tipo_pantalla*) &juego.arkanoPi.pantalla,
					(tipo_pantalla*) &inicial);
			PintaPantallaPorTerminal((tipo_pantalla*) &juego.arkanoPi.pantalla);
			printf("Game Over\n");
			printf("Ladrillos restantes = %d \n", ladrillosQuedan);
		} else {
			int matrizGanar[MATRIZ_ANCHO][MATRIZ_ALTO] = {
					{ 0, 0, 0, 0, 0, 0, 0 }, { 1, 1, 0, 0, 0, 0, 0 }, { 1, 1, 0,
							0, 1, 0, 0 }, { 0, 0, 0, 0, 0, 1, 0 }, { 0, 0, 0, 0,
							0, 1, 0 }, { 0, 0, 0, 0, 0, 1, 0 }, { 0, 0, 0, 0, 0,
							1, 0 }, { 1, 1, 0, 0, 1, 0, 0 }, { 1, 1, 0, 0, 0, 0,
							0 }, { 0, 0, 0, 0, 0, 0, 0 }, };

			int i, j = 0;

			for (i = 0; i < MATRIZ_ANCHO; i++) {
				for (j = 0; j < MATRIZ_ALTO; j++) {
					inicial.matriz[i][j] = matrizGanar[i][j];
				}
			}
			PintaMensajeInicialPantalla(
					(tipo_pantalla*) &juego.arkanoPi.pantalla,
					(tipo_pantalla*) &inicial);
			PintaPantallaPorTerminal((tipo_pantalla*) &juego.arkanoPi.pantalla);
			printf("Victory \n");
		}
		piUnlock(STD_IO_BUFFER_KEY);
		tmr_destroy(tmr_pelota);
	} else if (juegoActual == 1) {
		piLock(STD_IO_BUFFER_KEY);
		if (juego2.arkanoPi_2p.pelota.y <= 0) {
			printf("Gana Player 1\n");
			int matrizGanaP1[MATRIZ_ANCHO][MATRIZ_ALTO] = { { 0, 0, 0, 0, 0, 1,
					1 }, { 0, 1, 0, 0, 1, 1, 1 }, { 1, 1, 1, 1, 1, 1, 1 }, { 0,
					0, 0, 0, 1, 1, 1 }, { 0, 0, 0, 0, 0, 1, 1 }, { 0, 0, 0, 0,
					0, 0, 0 }, { 0, 0, 1, 0, 1, 1, 1 }, { 0, 0, 1, 0, 1, 0, 1 },
					{ 0, 0, 1, 1, 1, 0, 1 }, { 0, 0, 0, 0, 0, 0, 0 }, };

			int i, j = 0;

			for (i = 0; i < MATRIZ_ANCHO; i++) {
				for (j = 0; j < MATRIZ_ALTO; j++) {
					inicial.matriz[i][j] = matrizGanaP1[i][j];
				}
			}
			PintaMensajeInicialPantalla(
					(tipo_pantalla*) &juego2.arkanoPi_2p.pantalla,
					(tipo_pantalla*) &inicial);
			PintaPantallaPorTerminal(
					(tipo_pantalla*) &juego2.arkanoPi_2p.pantalla);
		} else {
			printf("Gana Player 2 \n");
			int matrizGanaP1[MATRIZ_ANCHO][MATRIZ_ALTO] = { { 0, 0, 0, 0, 0, 1,
					1 }, { 1, 0, 1, 1, 1, 1, 1 }, { 1, 0, 1, 0, 1, 1, 1 }, { 1,
					1, 1, 0, 1, 1, 1 }, { 0, 0, 0, 0, 0, 1, 1 }, { 0, 0, 0, 0,
					0, 0, 0 }, { 0, 0, 0, 1, 0, 0, 1 }, { 0, 0, 1, 1, 1, 1, 1 },
					{ 0, 0, 0, 0, 0, 0, 1 }, { 0, 0, 0, 0, 0, 0, 0 }, };

			int i, j = 0;

			for (i = 0; i < MATRIZ_ANCHO; i++) {
				for (j = 0; j < MATRIZ_ALTO; j++) {
					inicial.matriz[i][j] = matrizGanaP1[i][j];
				}
			}
			PintaMensajeInicialPantalla(
					(tipo_pantalla*) &juego2.arkanoPi_2p.pantalla,
					(tipo_pantalla*) &inicial);
			PintaPantallaPorTerminal(
					(tipo_pantalla*) &juego2.arkanoPi_2p.pantalla);
		}
		piUnlock(STD_IO_BUFFER_KEY);
		tmr_destroy(tmr_pelota);
	} else {

	}

}


/**
 * @brief			funcion encargada de llevar a cabo la reinicializacion de variables o estructuras.
 * @param	fsm		Maquina de estados
 */

void ReseteaJuego(fsm_t* fsm) {
	piLock(FLAGS_KEY);
	flags &= ~FLAG_TECLA;
	flags &= ~FLAG_PELOTA;
	flags &= ~FLAG_RAQUETA_DERECHA;
	flags &= ~FLAG_RAQUETA_IZQUIERDA;
	flags &= ~FLAG_FINAL_JUEGO;
	flags &= ~FLAG_JOYSTICK;
	flags &= ~FLAG_PAUSE;
	piUnlock(FLAGS_KEY);

	piLock(STD_IO_BUFFER_KEY);
	int matrizInicial[MATRIZ_ANCHO][MATRIZ_ALTO] = { { 1, 1, 1, 1, 1, 1, 1 }, {
			0, 0, 0, 1, 0, 0, 0 }, { 0, 0, 0, 1, 0, 0, 0 }, { 1, 1, 1, 1, 1, 1,
			1 }, { 0, 0, 0, 0, 0, 0, 0 }, { 1, 0, 0, 0, 0, 0, 1 }, { 1, 1, 1, 1,
			1, 1, 1 }, { 1, 1, 1, 1, 1, 1, 1 }, { 1, 0, 0, 0, 0, 0, 1 }, { 0, 0,
			0, 0, 0, 0, 0 }, };

	int i, j = 0;

	for (i = 0; i < MATRIZ_ANCHO; i++) {
		for (j = 0; j < MATRIZ_ALTO; j++) {
			inicial.matriz[i][j] = matrizInicial[i][j];
		}
	}

	if (juegoActual == 0) {
		PintaMensajeInicialPantalla((tipo_pantalla*) &juego.arkanoPi.pantalla,
				(tipo_pantalla*) &inicial);
		PintaPantallaPorTerminal((tipo_pantalla*) &juego.arkanoPi.pantalla);
	} else if (juegoActual == 1) {
		PintaMensajeInicialPantalla(
				(tipo_pantalla*) &juego2.arkanoPi_2p.pantalla,
				(tipo_pantalla*) &inicial);
		PintaPantallaPorTerminal((tipo_pantalla*) &juego2.arkanoPi_2p.pantalla);
	} else {
		printf("ERROR!! NO HAY JUEGO :( \n");
	}
	piUnlock(STD_IO_BUFFER_KEY);
}

/**
 * @brief			Controla la posición del Joystick
 * @param	this	Máquina de estados
 */
void controlJoystick(fsm_t* fsm) {
	piLock(FLAGS_KEY);
	flags &= ~FLAG_JOYSTICK;
	piUnlock(FLAGS_KEY);
	//Configuración ADC
	unsigned char ByteSPI[3];
	float voltaje = 0.0;
	ByteSPI[0] = 0b10011111;
	ByteSPI[1] = 0b0;
	ByteSPI[2] = 0b0;

	wiringPiSPIDataRW(SPI_ADC_CH, ByteSPI, 3);
	usleep(20);
	int salida_SPI = ((ByteSPI[1] << 5) | (ByteSPI[2] >> 3)) & 0xFFF;

	voltaje = 60 * (((float) salida_SPI) / 1500); //Entre 0 y 60 (por facilidad)
	//Elegimos posición raqueta
	if (juegoActual == 0) {
		if (voltaje < 5) {
			juego.arkanoPi.raqueta.x = -2;
		} else if (voltaje < 10) {
			juego.arkanoPi.raqueta.x = -1;
		} else if (voltaje < 15) {
			juego.arkanoPi.raqueta.x = 0;
		} else if (voltaje < 20) {
			juego.arkanoPi.raqueta.x = 1;
		} else if (voltaje < 25) {
			juego.arkanoPi.raqueta.x = 2;
		} else if (voltaje < 30) {
			juego.arkanoPi.raqueta.x = 3;
		} else if (voltaje < 35) {
			juego.arkanoPi.raqueta.x = 4;
		} else if (voltaje < 40) {
			juego.arkanoPi.raqueta.x = 5;
		} else if (voltaje < 45) {
			juego.arkanoPi.raqueta.x = 6;
		} else if (voltaje < 50) {
			juego.arkanoPi.raqueta.x = 7;
		} else if (voltaje < 55) {
			juego.arkanoPi.raqueta.x = 8;
		} else {
			juego.arkanoPi.raqueta.x = 9;
		}
	}
	if (juegoActual == 1) {
		if (voltaje < 5) {
			juego2.arkanoPi_2p.raqueta1.x = -2;
		} else if (voltaje < 10) {
			juego2.arkanoPi_2p.raqueta1.x = -1;
		} else if (voltaje < 15) {
			juego2.arkanoPi_2p.raqueta1.x = 0;
		} else if (voltaje < 20) {
			juego2.arkanoPi_2p.raqueta1.x = 1;
		} else if (voltaje < 25) {
			juego2.arkanoPi_2p.raqueta1.x = 2;
		} else if (voltaje < 30) {
			juego2.arkanoPi_2p.raqueta1.x = 3;
		} else if (voltaje < 35) {
			juego2.arkanoPi_2p.raqueta1.x = 4;
		} else if (voltaje < 40) {
			juego2.arkanoPi_2p.raqueta1.x = 5;
		} else if (voltaje < 45) {
			juego2.arkanoPi_2p.raqueta1.x = 6;
		} else if (voltaje < 50) {
			juego2.arkanoPi_2p.raqueta1.x = 7;
		} else if (voltaje < 55) {
			juego2.arkanoPi_2p.raqueta1.x = 8;
		} else {
			juego2.arkanoPi_2p.raqueta1.x = 9;
		}
	} else {
		printf("ERROR!! NO HAY JUEGO\n");
	}

}
/**
 * @brief			Pausa el juego cuando pulsamos los dos pulsadores
 * @param	this	Máquina de estados
 * @note			Lo controla el FLAG_PAUSE
 */
void PausaJuego(fsm_t* fsm) {
	piLock(FLAGS_KEY);
	flags &= ~FLAG_TECLA;
	flags &= ~FLAG_PELOTA;
	flags &= ~FLAG_RAQUETA_DERECHA;
	flags &= ~FLAG_RAQUETA_IZQUIERDA;
	flags &= ~FLAG_FINAL_JUEGO;
	flags &= ~FLAG_JOYSTICK;
	flags &= ~FLAG_PAUSE;
	piUnlock(FLAGS_KEY);

	if (millis() < debounceTime) {
		debounceTime = millis() + DEBOUNCE_TIME;
	}

	piLock(STD_IO_BUFFER_KEY);
	//Matriz que usamos para pintar el pause (||)
	int matrizInicial[MATRIZ_ANCHO][MATRIZ_ALTO] = { { 0, 0, 0, 0, 0, 0, 0 }, {
			0, 0, 0, 0, 0, 0, 0 }, { 1, 1, 1, 1, 1, 1, 1 }, { 1, 1, 1, 1, 1, 1,
			1 }, { 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0 }, { 1, 1, 1, 1,
			1, 1, 1 }, { 1, 1, 1, 1, 1, 1, 1 }, { 0, 0, 0, 0, 0, 0, 0 }, { 0, 0,
			0, 0, 0, 0, 0 }, };

	int i, j = 0;

	for (i = 0; i < MATRIZ_ANCHO; i++) {
		for (j = 0; j < MATRIZ_ALTO; j++) {
			inicial.matriz[i][j] = matrizInicial[i][j];
		}
	}

	if (juegoActual == 0) {
		PintaMensajeInicialPantalla((tipo_pantalla*) &juego.arkanoPi.pantalla,
				(tipo_pantalla*) &inicial);
		PintaPantallaPorTerminal((tipo_pantalla*) &juego.arkanoPi.pantalla);
	} else if (juegoActual == 1) {
		PintaMensajeInicialPantalla(
				(tipo_pantalla*) &juego2.arkanoPi_2p.pantalla,
				(tipo_pantalla*) &inicial);
		PintaPantallaPorTerminal((tipo_pantalla*) &juego2.arkanoPi_2p.pantalla);
	} else {
		printf("ERROR!! NO HAY JUEGO SELECCIONADO\n");
	}

	while ((digitalRead(PIN_BOTON_DCHA) == HIGH)
			&& (digitalRead(PIN_BOTON_IZDA) == HIGH)) {
		delay(1);
	}
	debounceTime = millis() + DEBOUNCE_TIME;
	piUnlock(STD_IO_BUFFER_KEY);

}

/**
 * @brief			Reanuda el juego cuando pulsamos los dos pulsadores
 * @param	this	Máquina de estados
 * @note			Lo controla el FLAG_PAUSE
 */
void ContinuaJuego(fsm_t* fsm) {
	piLock(FLAGS_KEY);
	flags &= ~FLAG_TECLA;
	flags &= ~FLAG_PELOTA;
	flags &= ~FLAG_RAQUETA_DERECHA;
	flags &= ~FLAG_RAQUETA_IZQUIERDA;
	flags &= ~FLAG_FINAL_JUEGO;
	flags &= ~FLAG_JOYSTICK;
	flags &= ~FLAG_PAUSE;
	piUnlock(FLAGS_KEY);

	if (millis() < debounceTime) {
		debounceTime = millis() + DEBOUNCE_TIME;
	}

	piLock(STD_IO_BUFFER_KEY);
	if (juegoActual == 0) {
		ActualizaPantalla((tipo_arkanoPi*) &juego.arkanoPi);
		PintaPantallaPorTerminal((tipo_pantalla*) &juego.arkanoPi.pantalla);
	} else if (juegoActual == 1) {
		ActualizaPantalla_2((tipo_arkanoPi_2p*) &juego2.arkanoPi_2p);
		PintaPantallaPorTerminal((tipo_pantalla*) &juego2.arkanoPi_2p.pantalla);
	} else {
		printf("ERROR!! NO HAY JUEGO :( \n");
	}
	//Antirrebotes
	while ((digitalRead(PIN_BOTON_DCHA) == HIGH)
			&& (digitalRead(PIN_BOTON_IZDA) == HIGH)) {
		delay(1);
	}
	debounceTime = millis() + DEBOUNCE_TIME;
	piUnlock(STD_IO_BUFFER_KEY);

}

//------------------------------------------------------
// FUNCIONES DEL SPI
//------------------------------------------------------

//------------------------------------------------------
// FUNCIONES DE INICIALIZACION
//------------------------------------------------------

/**
 * @brief			procedimiento de configuracion del sistema.
 * @return	int		1 si se inicio correctamente
 */

int systemSetup(void) {
	int x = 0;

	piLock(STD_IO_BUFFER_KEY);

	//sets up the wiringPi library
	if (wiringPiSetupGpio() < 0) {
		printf("Unable to setup wiringPi\n");
		piUnlock(STD_IO_BUFFER_KEY);
		return -1;
	}

	if (wiringPiSPISetup(SPI_ADC_CH, SPI_ADC_FREQ) < 0) {
		printf("No se pudo inicializar el dispositivo SPI (CH 0)\n");
		exit(1);
		return -2;
	}

	//Lanzamos Thread para exploracion del teclado convencional del PC
	//x = piThreadCreate(thread_explora_teclado);

	if (x != 0) {

		piUnlock(STD_IO_BUFFER_KEY);
		return -1;
	}

	piUnlock(STD_IO_BUFFER_KEY);
	return 1;
}

/**
 * @brief			Inicialización de la máquina de estados
 * @param	this	Máquina de estados
 */
void fsm_setup(fsm_t* fsm) {
	piLock(FLAGS_KEY);
	flags = 0;
	piUnlock(FLAGS_KEY);

	ReseteaJuego(fsm);

	piLock(STD_IO_BUFFER_KEY);
	piUnlock(STD_IO_BUFFER_KEY);
}

//-------------------------------------------------
//PI_THREAD(thread_explora_teclado): thread para detectar e interpretar pulsaciones de teclas
//-------------------------------------------------

//-----------------------------------------
//TEMPORIZACI�N DEL SISTEMA
//-----------------------------------------

/**
 * @brief			Inicialización pines
 */
void inicializaPines() {
	//Filas
	pinMode(0, OUTPUT);
	pinMode(1, OUTPUT);
	pinMode(2, OUTPUT);
	pinMode(3, OUTPUT);
	pinMode(4, OUTPUT);
	pinMode(7, OUTPUT);
	pinMode(23, OUTPUT);

	//Columnas
	pinMode(14, OUTPUT);
	pinMode(17, OUTPUT);
	pinMode(18, OUTPUT);
	pinMode(22, OUTPUT);

	//Pulsadores
	pinMode(PIN_BOTON_DCHA, INPUT);
	pinMode(PIN_BOTON_IZDA, INPUT);
	pinMode(PIN_BOTON_SUBMENU, INPUT);

}

/**
 * @brief			Exploración matriz para pintarla con los leds
 */
static void refresca_matriz(union sigval arg) {
	//DEFINICIÓN DE PINES DE LA RASPI
	int i;

	//Escritura de las filas

	if (juegoActual == 0) {
		piLock(STD_IO_BUFFER_KEY);
		for (i = 0; i < MATRIZ_ALTO; i++) {
			if ((juego.arkanoPi.pantalla.matriz[columna][i] >= 1)) { // ¿o [9-columna]?
				switch (i) {
				case 5:
					digitalWrite(7, LOW);
					break;
				case 6:
					digitalWrite(23, LOW);
					break;
				default:
					digitalWrite(i, LOW);
					break;
				}
			} else {
				switch (i) {
				case 5:
					digitalWrite(7, HIGH);
					break;
				case 6:
					digitalWrite(23, HIGH);
					break;
				default:
					digitalWrite(i, HIGH);
					break;

				}
			}
		}
		//Escritura en las columnas(número binario)
		digitalWrite(14, columna & 0x01);
		digitalWrite(17, columna & 0x02);
		digitalWrite(18, columna & 0x04);
		digitalWrite(22, columna & 0x08);
		piUnlock(STD_IO_BUFFER_KEY);
		columna++; //Actualizo para que en la siguiente cuenta salte de columna
		if (columna >= 10)
			columna = 0;
		tmr_startms(tmr, 1);
	} else if (juegoActual == 1) {
		piLock(STD_IO_BUFFER_KEY);
		for (i = 0; i < MATRIZ_ALTO; i++) {
			if ((juego2.arkanoPi_2p.pantalla.matriz[columna][i] >= 1)) { // ¿o [9-columna]?
				switch (i) {
				case 5:
					digitalWrite(7, LOW);
					break;
				case 6:
					digitalWrite(23, LOW);
					break;
				default:
					digitalWrite(i, LOW);
					break;
				}
			} else {
				switch (i) {
				case 5:
					digitalWrite(7, HIGH);
					break;
				case 6:
					digitalWrite(23, HIGH);
					break;
				default:
					digitalWrite(i, HIGH);
					break;

				}
			}
		}
		//Escritura en las columnas(número binario)
		digitalWrite(14, columna & 0x01);
		digitalWrite(17, columna & 0x02);
		digitalWrite(18, columna & 0x04);
		digitalWrite(22, columna & 0x08);
		piUnlock(STD_IO_BUFFER_KEY);
		columna++; //Actualizo para que en la siguiente cuenta salte de columna
		if (columna >= 10)
			columna = 0;
		tmr_startms(tmr, 1);
	} else {
	}

}

//-----------------------------------------
//ACTIVACIÓN FLAGS
//-----------------------------------------

/**
 * @brief			Arranca el temporizador de pelota y activa el FLAG_PELOTA
 */
static void refresca_pelota(union sigval arg) {
	piLock(FLAGS_KEY);
	flags |= FLAG_PELOTA;
	piUnlock(FLAGS_KEY);
	tmr_startms(tmr_pelota, 500);
}

/**
 * @brief			Arranca el temporizador de joystick y activa el FLAG_JOYSTICK
 */
static void refresca_joystick(union sigval arg) {
	piLock(FLAGS_KEY);
	flags |= FLAG_JOYSTICK;
	piUnlock(FLAGS_KEY);
	tmr_startms(tmr_joystick, 100);
}

/**
 * @brief			Activa el FLAG_PAUSE, o el FLAG_RAQUETA_DERECHA y el FLAG_TECLA
 */
void botonDerecha(void) {
	pausaDerecha = millis();
	//Comprobamos si se pulsó a la vez que el izquierdo ---> Pause
	if ((pausaDerecha - pausaIzquierda) < PAUSE_TIME && pausaIzquierda != 0) {
		piLock(FLAGS_KEY);
		flags |= FLAG_TECLA;
		flags |= FLAG_PAUSE;
		piUnlock(FLAGS_KEY);
	} else {
		piLock(FLAGS_KEY);
		flags |= FLAG_TECLA;
		flags |= FLAG_RAQUETA_DERECHA;
		piUnlock(FLAGS_KEY);
	}
}

/**
 * @brief			Activa el FLAG_PAUSE, o el FLAG_RAQUETA_IZQ y el FLAG_TECLA
 */
void botonIzquierda(void) {
	pausaIzquierda = millis();

	if ((pausaIzquierda - pausaDerecha) < PAUSE_TIME && pausaDerecha != 0) {
		piLock(FLAGS_KEY);
		flags |= FLAG_TECLA;
		flags |= FLAG_PAUSE;
		piUnlock(FLAGS_KEY);
	} else {
		piLock(FLAGS_KEY);
		flags |= FLAG_TECLA;
		flags |= FLAG_RAQUETA_IZQUIERDA;
		piUnlock(FLAGS_KEY);
	}
}

/**
 * @brief			Activa el FLAG_PAUSE, o el FLAG_RAQUETA_IZQ y el FLAG_TECLA
 */
void botonSubmenu(void) {

	piLock(FLAGS_KEY);
	flags |= FLAG_TECLA;
	flags |= FLAG_SUBMENU;
	piUnlock(FLAGS_KEY);

}


/**
 * @brief			Metodo main que arranca el juego
 */
int main() {
	tmr = tmr_new(refresca_matriz);
	unsigned int next;

	//Maquina de estados: lista de transiciones
	//{EstadoOrigen,FuncionEntrada,EstadoDestino,FuncionSalida}
	fsm_trans_t estado_juego[] = { { WAIT_START, CompruebaTeclaPulsada, MENU,
			abreMenu }, { MENU, CompruebaTeclaRaquetaIzquierda, SUBMENU,
			abreSubmenu1 }, { SUBMENU, CompruebaTeclaRaquetaIzquierda, SUBMENU,
			abreSubmenu1 }, { SUBMENU, CompruebaTeclaRaquetaDerecha, SUBMENU,
			abreSubmenu2 }, { SUBMENU, CompruebaTeclaSubmenu, WAIT_PUSH,
			InicializaJuego }, { MENU, CompruebaTeclaRaquetaDerecha, WAIT_PUSH,
			InicializaJuego_2 }, { WAIT_PUSH, CompruebaTeclaPelota, WAIT_PUSH,
			MovimientoPelota }, { WAIT_PUSH, CompruebaTeclaRaquetaDerecha,
			WAIT_PUSH, MueveRaquetaDerecha }, { WAIT_PUSH,
			CompruebaTeclaRaquetaIzquierda, WAIT_PUSH, MueveRaquetaIzquierda },
			{ WAIT_PUSH, CompruebaJoystick, WAIT_PUSH, controlJoystick }, {
					WAIT_PUSH, CompruebaFinalJuego, WAIT_END, FinalJuego }, {
					WAIT_END, CompruebaTeclaPulsada, WAIT_START, ReseteaJuego },
			{ WAIT_PUSH, CompruebaPause, PAUSE, PausaJuego }, { PAUSE,
					CompruebaPause, WAIT_PUSH, ContinuaJuego }, { -1, NULL, -1,
					NULL }, };

	fsm_t* juego_fsm = fsm_new(WAIT_START, estado_juego, NULL);

	//Configuracion e inicializacion del sistema
	systemSetup();
	fsm_setup(juego_fsm);

	//Inicializacion interrupciones pulsadores
	wiringPiISR(PIN_BOTON_DCHA, INT_EDGE_FALLING, botonDerecha);
	wiringPiISR(PIN_BOTON_IZDA, INT_EDGE_FALLING, botonIzquierda);
	wiringPiISR(PIN_BOTON_SUBMENU, INT_EDGE_FALLING, botonSubmenu);

	next = millis();
	inicializaPines();
	tmr_startms(tmr, 1);
	while (1) {
		fsm_fire(juego_fsm);
		next += CLK_MS;
		delay_until(next);

	}
	fsm_destroy(juego_fsm);
}

