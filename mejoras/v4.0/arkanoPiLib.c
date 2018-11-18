/** File Name          : arkanoPi_1.c
  * Description        : Incluye metodos de inicialización de matrices de juego
  */

/* Includes ------------------------------------------------------------------*/
#include "arkanoPiLib.h"


/* Private variables ---------------------------------------------------------*/
int ladrillos_basico[MATRIZ_ANCHO][MATRIZ_ALTO] = {
		        {1,1,0,0,0,0,0},
				{1,1,0,0,0,0,0},
				{1,1,0,0,0,0,0},
				{1,1,0,0,0,0,0},
				{1,1,0,0,0,0,0},
				{1,1,0,0,0,0,0},
				{1,1,0,0,0,0,0},
				{1,1,0,0,0,0,0},
				{1,1,0,0,0,0,0},
				{1,1,0,0,0,0,0},
};
//Ladrillos que se derriban con dos impactos
int ladrillos_avanzados[MATRIZ_ANCHO][MATRIZ_ALTO] = {
		{2,2,0,0,0,0,0},
		{2,2,0,0,0,0,0},
		{2,2,0,0,0,0,0},
		{2,2,0,0,0,0,0},
		{2,2,0,0,0,0,0},
		{2,2,0,0,0,0,0},
		{2,2,0,0,0,0,0},
		{2,2,0,0,0,0,0},
		{2,2,0,0,0,0,0},
		{2,2,0,0,0,0,0},
};

tipo_pantalla pantalla_inicial={
	    .matriz={

        {0,0,0,0,0,0,0},
		{0,1,0,0,0,1,0},
		{0,1,1,1,1,1,0},
		{0,1,0,0,0,1,0},
		{0,0,0,0,0,0,0},
		{0,1,1,1,1,1,0},
		{0,0,0,1,0,0,0},
		{0,1,1,1,1,1,0},
		{0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0}
}
};





//------------------------------------------------------
// FUNCIONES DE INICIALIZACION / RESET
//------------------------------------------------------

/**
 * @brief			Función que pone con todos ceros la pantalla
 * @param	*p_pantalla	Puntero a pantalla
 */
void ReseteaMatriz(tipo_pantalla *p_pantalla) {
	int i, j = 0;

	for(i=0;i<MATRIZ_ANCHO;i++) {
		for(j=0;j<MATRIZ_ALTO;j++) {
			p_pantalla->matriz[i][j] = 0;
		}
	}
}

/**
 * @brief			Función que pone con todos ceros los ladrillos
 * @param	*p_pantalla	Puntero a pantalla
 * @param masDisparos	Indica cuantos disparos adicionales son necesarios para derribar un ladrillo
 */
void ReseteaLadrillos(tipo_pantalla *p_ladrillos, int masDisparos) {
	int i, j = 0;

	for(i=0;i<MATRIZ_ANCHO;i++) {
		for(j=0;j<MATRIZ_ALTO;j++) {
			if (masDisparos == 0)
				p_ladrillos->matriz[i][j] = ladrillos_basico[i][j];
			else
				p_ladrillos->matriz[i][j] = ladrillos_avanzados[i][j];

		}
	}
}

/**
 * @brief			Función que reinicia la pelota
 * @param	*p_pantalla	Puntero a pantalla
 */
void ReseteaPelota(tipo_pelota *p_pelota) {
	// Pelota inicialmente en el centro de la pantalla
	p_pelota->x = MATRIZ_ANCHO/2 - 1;
	p_pelota->y = MATRIZ_ALTO/2 - 1;

	// Trayectoria inicial
	p_pelota->yv = 1;
	p_pelota->xv = 0;
}

/**
 * @brief			Función que reinicia la raqueta
 * @param	*p_pantalla	Puntero a pantalla
 */
void ReseteaRaqueta(tipo_raqueta *p_raqueta) {
	// Raqueta inicialmente en el centro de la pantalla
	p_raqueta->ancho = RAQUETA_ANCHO;
    p_raqueta->alto = RAQUETA_ALTO;

	p_raqueta->x = MATRIZ_ANCHO/2 - p_raqueta->ancho/2;
	p_raqueta->y = MATRIZ_ALTO - 1;

}

/**
 * @brief			Función que reinicia la raqueta del Pong 1
 * @param	*p_pantalla	Puntero a pantalla
 */
void ReseteaRaqueta_1(tipo_raqueta *p_raqueta) {
	// Raqueta inicialmente en el centro de la pantalla
	p_raqueta->ancho = RAQUETA_ANCHO;
    p_raqueta->alto = RAQUETA_ALTO;

	p_raqueta->x = MATRIZ_ANCHO/2 - p_raqueta->ancho/2;
	p_raqueta->y = MATRIZ_ALTO - 1;

}

/**
 * @brief			Función que reinicia la raqueta del Pong 2
 * @param	*p_pantalla	Puntero a pantalla
 */
void ReseteaRaqueta_2(tipo_raqueta *p_raqueta) {
	// Raqueta inicialmente en el centro de la pantalla
	p_raqueta->ancho = RAQUETA_ANCHO;
    p_raqueta->alto = RAQUETA_ALTO;

	p_raqueta->x = MATRIZ_ANCHO/2 - p_raqueta->ancho/2;
	p_raqueta->y = 0;
}

//------------------------------------------------------
// FUNCIONES DE VISUALIZACION (ACTUALIZACION DEL OBJETO PANTALLA QUE LUEGO USARA EL DISPLAY)
//------------------------------------------------------

/**
 * @brief			Función que pone en la matriz pantalla la inicial
 * @param	*p_pantalla	Puntero a pantalla
 * @param	*p_pantalla_inicial	Puntero a pantalla inicial
 */
void PintaMensajeInicialPantalla (tipo_pantalla *p_pantalla, tipo_pantalla *p_pantalla_inicial) {
	// A completar por el alumno...
   // ya preparado para mostrar en display, NO EN TERMINAL
	int k,l;
     //ReseteaMatriz
	for(k=0;k<MATRIZ_ANCHO;k++) {
		for(l=0;l<MATRIZ_ALTO;l++) {
			p_pantalla->matriz[k][l] = p_pantalla_inicial->matriz[k][l];

		}
	}
 // debugging


}



/**
 * @brief			Función que pinta la pantalla por el terminal
 * @param	*p_pantalla	Puntero a pantalla
 */
void PintaPantallaPorTerminal  (tipo_pantalla *p_pantalla) {

	int i,j;

	for(i=0;i<MATRIZ_ALTO;i++){
		for(j=0;j<MATRIZ_ANCHO;j++) {
			printf(" %d ",p_pantalla->matriz[j][i]);
			fflush(stdout);
		}
		printf("\n");
		fflush(stdout);
	}
	printf("\n");
}




/**
 * @brief			Función que pone en la matriz pantalla los ladrillos
 * @param	*p_pantalla	Puntero a pantalla
 * @param	*p_ladrillos	Puntero a ladrillos
 */
void PintaLadrillos(tipo_pantalla *p_ladrillos, tipo_pantalla *p_pantalla) {
	int i, j = 0;

	for(i=0;i<MATRIZ_ANCHO;i++) {
		for(j=0;j<MATRIZ_ALTO;j++) {
			p_pantalla->matriz[i][j] = p_ladrillos->matriz[i][j];
		}
	}
}

/**
 * @brief			Función que pone en la matriz pantalla la raqueta
 * @param	*p_pantalla	Puntero a pantalla
 * @param	*p_pantalla_inicial	Puntero a pantalla raqueta
 */
void PintaRaqueta(tipo_raqueta *p_raqueta, tipo_pantalla *p_pantalla) {
	int i, j = 0;

	for(i=0;i<RAQUETA_ANCHO;i++) {
		for(j=0;j<RAQUETA_ALTO;j++) {
			if (( (p_raqueta->x+i >= 0) && (p_raqueta->x+i < MATRIZ_ANCHO) ) &&
					( (p_raqueta->y+j >= 0) && (p_raqueta->y+j < MATRIZ_ALTO) ))
				p_pantalla->matriz[p_raqueta->x+i][p_raqueta->y+j] = 1;
		}
	}
}

/**
 * @brief			Función que pone en la matriz pantalla la pelota
 * @param	*p_pantalla	Puntero a pantalla
 * @param	*p_pelota	Puntero a pelota
 */
void PintaPelota(tipo_pelota *p_pelota, tipo_pantalla *p_pantalla) {
	if( (p_pelota->x >= 0) && (p_pelota->x < MATRIZ_ANCHO) ) {
		if( (p_pelota->y >= 0) && (p_pelota->y < MATRIZ_ALTO) ) {
			p_pantalla->matriz[p_pelota->x][p_pelota->y] = 1;
		}
		else {
			printf("\n\nPROBLEMAS!!!! posicion y=%d de la pelota INVALIDA!!!\n\n", p_pelota->y);
			fflush(stdout);
		}
	}
	else {
		printf("\n\nPROBLEMAS!!!! posicion x=%d de la pelota INVALIDA!!!\n\n", p_pelota->x);
		fflush(stdout);
	}
}

/**
 * @brief			Función que acutaliza la pantalla con los ladrillos, pelota y raqueta
 * @param	*p_arkanoPI	Puntero al arkanoPi
 */
void ActualizaPantalla(tipo_arkanoPi* p_arkanoPi) {
	// Borro toda la pantalla
	ReseteaMatriz((tipo_pantalla*)(&(p_arkanoPi->pantalla)));

	PintaLadrillos(&p_arkanoPi->ladrillos,&p_arkanoPi->pantalla);
	PintaRaqueta(&p_arkanoPi->raqueta,&p_arkanoPi->pantalla);
	PintaPelota(&p_arkanoPi->pelota,&p_arkanoPi->pantalla);

}

/**
 * @brief			Función que acutaliza la pantalla con la pelota y raqueta para el Pong
 * @param	*p_arkanoPI	Puntero al arkanoPi
 */
void ActualizaPantalla_2(tipo_arkanoPi_2p* p_arkanoPi) {
	// Borro toda la pantalla
	ReseteaMatriz((tipo_pantalla*)(&(p_arkanoPi->pantalla)));



	PintaRaqueta(&p_arkanoPi->raqueta1,&p_arkanoPi->pantalla);
	PintaRaqueta(&p_arkanoPi->raqueta2,&p_arkanoPi->pantalla);
	PintaPelota(&p_arkanoPi->pelota,&p_arkanoPi->pantalla);

}

/**
 * @brief			Función que inicializa arkanoPi
 * @param	*p_arkanoPI	Puntero al arkanoPi
 * @param 	masDisparos	Número de disparos adicionales para derribar un ladrillo
 */
void InicializaArkanoPi(tipo_arkanoPi *p_arkanoPi, int masDisparos) {

	ReseteaMatriz((tipo_pantalla*)(&(p_arkanoPi->pantalla)));

	ReseteaLadrillos((tipo_pantalla*)(&(p_arkanoPi->ladrillos)), masDisparos);
	ReseteaPelota((tipo_pelota*)(&(p_arkanoPi->pelota)));
	ReseteaRaqueta((tipo_raqueta*)(&(p_arkanoPi->raqueta)));

	.
	PintaLadrillos(&p_arkanoPi->ladrillos,&p_arkanoPi->pantalla);
	PintaRaqueta(&p_arkanoPi->raqueta,&p_arkanoPi->pantalla);
	PintaPelota(&p_arkanoPi->pelota,&p_arkanoPi->pantalla);

}

/**
 * @brief			Función que inicializa Pong
 * @param	*p_arkanoPI	Puntero al arkanoPi
 */
void InicializaArkanoPi_2(tipo_arkanoPi_2p *p_arkanoPi) {

	ReseteaMatriz((tipo_pantalla*)(&(p_arkanoPi->pantalla)));

	ReseteaPelota((tipo_pelota*)(&(p_arkanoPi->pelota)));
	ReseteaRaqueta_1((tipo_raqueta*)(&(p_arkanoPi->raqueta1)));
	ReseteaRaqueta_2((tipo_raqueta*)(&(p_arkanoPi->raqueta2)));

	PintaRaqueta(&p_arkanoPi->raqueta1,&p_arkanoPi->pantalla);
	PintaRaqueta(&p_arkanoPi->raqueta2,&p_arkanoPi->pantalla);
	PintaPelota(&p_arkanoPi->pelota,&p_arkanoPi->pantalla);

}



/**
 * @brief			Función que devuelve el número de ladrillos que quedan
 * @param	*p_ladrillos	Puntero a ladrillos
 * @return 		Número de ladrillos que quedan
 */
int CalculaLadrillosRestantes(tipo_pantalla *p_ladrillos) {
	int num_ladrillos_restantes = 0;
	int i,j;

	for(i=0;i<LADRILLOS_ANCHO;i++) {
		for(j=0;j<LADRILLOS_ALTO;j++) {

			if (p_ladrillos->matriz[i][j] >=1){
				num_ladrillos_restantes=num_ladrillos_restantes+1;

			}
		}
	}


	return num_ladrillos_restantes;
}
