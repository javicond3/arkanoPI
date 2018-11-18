#include "arkanoPi_1.h"
#include "kbhit.h" //Para detectar teclas pulsadas sin bloqueo y leer las teclas pulsadas
#include "fsm.h" //Para poder crear y ejecutar la maquina de estados
#include "time.h"
#include "tmr.h"
#include "wiringPi.h"

#define CLK_MS 10 // PERIODO DE ACTUALIZACION DE LA MAQUINA DE ESTADOS

// FLAGS DEL SISTEMA
#define FLAG_TECLA 0x01
#define FLAG_PELOTA 0x02
#define FLAG_RAQUETA_DERECHA 0x04
#define FLAG_RAQUETA_IZQUIERDA 0x08
#define FLAG_FINAL_JUEGO 0x10

#define PIN_BOTON_DCHA 16
#define PIN_BOTON_IZDA 19
#define DEBOUNCE_TIME 200

volatile int flags = 0;

//mutex para los 2 procesos
#define FLAGS_KEY 1
#define STD_IO_BUFFER_KEY 2

static volatile tipo_juego juego;
tipo_pantalla inicial;
static tmr_t* tmr;
static tmr_t* tmr_pelota;
int columna = 0;
int debounceTime;




//------------------------------------------------------
//FUNCIONES DE LA MAQUINA DE ESTADOS
//------------------------------------------------------

void delay_until (unsigned int next) {
    unsigned int now = millis();

    if(next>now) {
        delay(next-now);
    }
}

int CompruebaTeclaPulsada(fsm_t* this){
    int result;

    piLock(FLAGS_KEY);
    result = (flags & FLAG_TECLA);
    piUnlock(FLAGS_KEY);

    return result;
}

int CompruebaTeclaPelota(fsm_t* this){
    int result;

    piLock(FLAGS_KEY);
    result = (flags & FLAG_PELOTA);
    piUnlock(FLAGS_KEY);

    return result;
}

int CompruebaTeclaRaquetaIzquierda(fsm_t* this){
    int result;

    piLock(FLAGS_KEY);
    result = (flags & FLAG_RAQUETA_IZQUIERDA);
    piUnlock(FLAGS_KEY);

    return result;
}

int CompruebaTeclaRaquetaDerecha(fsm_t* this){
    int result;

    piLock(FLAGS_KEY);
    result = (flags & FLAG_RAQUETA_DERECHA);
    piUnlock(FLAGS_KEY);

    return result;
}

int CompruebaFinalJuego(fsm_t* this){
    int result;

    piLock(FLAGS_KEY);
    result = (flags & FLAG_FINAL_JUEGO);
    piUnlock(FLAGS_KEY);

    return result;
}


//------------------------------------------------------
// FUNCIONES DE ACCION
//------------------------------------------------------

// void InicializaJuego (void): funcion encargada de llevar a cabo
// la oportuna inicializaci�n de toda variable o estructura de datos
// que resulte necesaria para el desarrollo del juego.
void InicializaJuego (fsm_t* fsm) {
    piLock(FLAGS_KEY);
    flags &= ~FLAG_TECLA;
    flags &= ~FLAG_PELOTA;
    flags &= ~FLAG_RAQUETA_DERECHA;
    flags &= ~FLAG_RAQUETA_IZQUIERDA;
    flags &= ~FLAG_FINAL_JUEGO;
    piUnlock(FLAGS_KEY);

    piLock(STD_IO_BUFFER_KEY);
    InicializaArkanoPi((tipo_arkanoPi*)&juego.arkanoPi);
    ActualizaPantalla((tipo_arkanoPi*) &juego.arkanoPi);
    PintaPantallaPorTerminal((tipo_pantalla*) &juego.arkanoPi.pantalla);
    piUnlock(STD_IO_BUFFER_KEY);

    tmr_pelota = tmr_new(refresca_pelota);
    tmr_startms(tmr_pelota,500);
}

// void MueveRaquetaIzquierda (void): funcion encargada de ejecutar
// el movimiento hacia la izquierda contemplado para la raqueta.
// Debe garantizar la viabilidad del mismo mediante la comprobaci�n
// de que la nueva posici�n correspondiente a la raqueta no suponga
// que �sta rebase o exceda los l�mites definidos para el �rea de juego
// (i.e. al menos uno de los leds que componen la raqueta debe permanecer
// visible durante todo el transcurso de la partida).
void MueveRaquetaIzquierda (fsm_t* fsm) {
    piLock(FLAGS_KEY);
    flags &= ~FLAG_TECLA;
    flags &= ~FLAG_PELOTA;
    flags &= ~FLAG_RAQUETA_DERECHA;
    flags &= ~FLAG_RAQUETA_IZQUIERDA;
    flags &= ~FLAG_FINAL_JUEGO;
    piUnlock(FLAGS_KEY);

    if (millis () < debounceTime){
    			debounceTime = millis () + DEBOUNCE_TIME;
    			return;
    }

    piLock(STD_IO_BUFFER_KEY);
    if(juego.arkanoPi.raqueta.x >-2)
        juego.arkanoPi.raqueta.x -= 1;
    ActualizaPantalla((tipo_arkanoPi*) &juego.arkanoPi);
    PintaPantallaPorTerminal((tipo_pantalla*) &juego.arkanoPi.pantalla);
    piUnlock(STD_IO_BUFFER_KEY);

    while(digitalRead(PIN_BOTON_DCHA) == HIGH){
    		delay(1);
    }
    debounceTime = millis () + DEBOUNCE_TIME;
}

// void MueveRaquetaDerecha (void): funci�n similar a la anterior
// encargada del movimiento hacia la derecha.
void MueveRaquetaDerecha (fsm_t* fsm) {
    piLock(FLAGS_KEY);
    flags &= ~FLAG_TECLA;
    flags &= ~FLAG_PELOTA;
    flags &= ~FLAG_RAQUETA_DERECHA;
    flags &= ~FLAG_RAQUETA_IZQUIERDA;
    flags &= ~FLAG_FINAL_JUEGO;
    piUnlock(FLAGS_KEY);

    if (millis () < debounceTime){
        	debounceTime = millis () + DEBOUNCE_TIME;
        	return;
    }

    piLock(STD_IO_BUFFER_KEY);
    if(juego.arkanoPi.raqueta.x < 9)
            juego.arkanoPi.raqueta.x += 1;
    ActualizaPantalla((tipo_arkanoPi*) &juego.arkanoPi);
    PintaPantallaPorTerminal((tipo_pantalla*) &juego.arkanoPi.pantalla);
    piUnlock(STD_IO_BUFFER_KEY);

    while(digitalRead(PIN_BOTON_DCHA) == HIGH){
        		delay(1);
    }
    debounceTime = millis () + DEBOUNCE_TIME;
}

// void MovimientoPelota (void): funci�n encargada de actualizar la
// posici�n de la pelota conforme a la trayectoria definida para �sta.
// Para ello deber� identificar los posibles rebotes de la pelota para,
// en ese caso, modificar su correspondiente trayectoria (los rebotes
// detectados contra alguno de los ladrillos implicar�n adicionalmente
// la eliminaci�n del ladrillo). Del mismo modo, deber� tambi�n
// identificar las situaciones en las que se d� por finalizada la partida:
// bien porque el jugador no consiga devolver la pelota, y por tanto �sta
// rebase el l�mite inferior del �rea de juego, bien porque se agoten
// los ladrillos visibles en el �rea de juego.
void MovimientoPelota (fsm_t* fsm) {

	piLock(FLAGS_KEY);
		flags &= ~FLAG_TECLA;
		flags &= ~FLAG_PELOTA;
		flags &= ~FLAG_RAQUETA_DERECHA;
		flags &= ~FLAG_RAQUETA_IZQUIERDA;
		flags &= ~FLAG_FINAL_JUEGO;
		piUnlock(FLAGS_KEY);


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


					piLock(FLAGS_KEY);
					flags |= FLAG_FINAL_JUEGO;
					piUnlock(FLAGS_KEY);
				}


			piUnlock(STD_IO_BUFFER_KEY);
}

// void FinalJuego (void): funci�n encargada de mostrar en la ventana de
// terminal los mensajes necesarios para informar acerca del resultado del juego.
void FinalJuego (fsm_t* fsm) {
    piLock(FLAGS_KEY);
    flags &= ~FLAG_TECLA;
    flags &= ~FLAG_PELOTA;
    flags &= ~FLAG_RAQUETA_DERECHA;
    flags &= ~FLAG_RAQUETA_IZQUIERDA;
    flags &= ~FLAG_FINAL_JUEGO;
    piUnlock(FLAGS_KEY);
    piLock(STD_IO_BUFFER_KEY);
    int ladrillosQuedan = CalculaLadrillosRestantes((tipo_pantalla*) &juego.arkanoPi.ladrillos);
    if(ladrillosQuedan != 0){
        printf("Game Over\n");
        printf("Ladrillos restantes = %d \n",ladrillosQuedan);
    }else{
        printf("Victory \n");
    }
    piUnlock(STD_IO_BUFFER_KEY);
    tmr_destroy(tmr_pelota);
}

//void ReseteaJuego (void): funci�n encargada de llevar a cabo la
// reinicializaci�n de cuantas variables o estructuras resulten
// necesarias para dar comienzo a una nueva partida.
void ReseteaJuego (fsm_t* fsm) {
    piLock(FLAGS_KEY);
    flags &= ~FLAG_TECLA;
    flags &= ~FLAG_PELOTA;
    flags &= ~FLAG_RAQUETA_DERECHA;
    flags &= ~FLAG_RAQUETA_IZQUIERDA;
    flags &= ~FLAG_FINAL_JUEGO;
    piUnlock(FLAGS_KEY);

    piLock(STD_IO_BUFFER_KEY);
    int matrizInicial[MATRIZ_ANCHO][MATRIZ_ALTO] = {
            {0,0,0,0,0,0,0},
            {0,0,0,0,0,0,0},
            {0,0,0,0,0,0,0},
            {0,0,0,0,0,0,0},
            {0,0,0,1,0,0,0},
            {0,0,0,0,0,0,0},
            {0,0,0,0,0,0,0},
            {0,0,0,0,0,0,0},
            {0,0,0,0,0,0,0},
            {0,0,0,0,0,0,0},
    };

    int i, j = 0;

    for(i=0;i<MATRIZ_ANCHO;i++) {
        for(j=0;j<MATRIZ_ALTO;j++) {
            inicial.matriz[i][j] = matrizInicial[i][j];
        }
    }
    PintaMensajeInicialPantalla((tipo_pantalla*) &juego.arkanoPi.pantalla, (tipo_pantalla*) &inicial);
    PintaPantallaPorTerminal((tipo_pantalla*) &juego.arkanoPi.pantalla);
    piUnlock(STD_IO_BUFFER_KEY);
}

//------------------------------------------------------
// FUNCIONES DE INICIALIZACION
//------------------------------------------------------

// int systemSetup (void): procedimiento de configuracion del sistema.
// Realizar�, entra otras, todas las operaciones necesarias para:
// configurar el uso de posibles librer�as (e.g. Wiring Pi),
// configurar las interrupciones externas asociadas a los pines GPIO,
// configurar las interrupciones peri�dicas y sus correspondientes temporizadores,
// crear, si fuese necesario, los threads adicionales que pueda requerir el sistema
int systemSetup (void) {
    int x = 0;

    piLock(STD_IO_BUFFER_KEY);

    //sets up the wiringPi library
    if(wiringPiSetupGpio()<0){
        printf("Unable to setup wiringPi\n");
        piUnlock(STD_IO_BUFFER_KEY);
        return -1;
    }

    //Lanzamos Thread para exploracion del teclado convencional del PC
    //x = piThreadCreate(thread_explora_teclado);

    if(x != 0) {
        printf("it didn't start!!!\n");
        piUnlock(STD_IO_BUFFER_KEY);
        return -1;
    }

    piUnlock(STD_IO_BUFFER_KEY);
    return 1;
}

void fsm_setup(fsm_t* fsm) {
    piLock(FLAGS_KEY);
    flags = 0;
    piUnlock(FLAGS_KEY);

    ReseteaJuego(fsm);

    piLock(STD_IO_BUFFER_KEY);
    printf("Pulsa tecla para iniciar\n");
    piUnlock(STD_IO_BUFFER_KEY);
}

//-------------------------------------------------
//PI_THREAD(thread_explora_teclado): thread para detectar e interpretar pulsaciones de teclas
//-------------------------------------------------

//-----------------------------------------
//TEMPORIZACI�N DEL SISTEMA
//-----------------------------------------
void inicializaPines(){
	//Filas
		pinMode(0,OUTPUT);
		pinMode(1,OUTPUT);
		pinMode(2,OUTPUT);
		pinMode(3,OUTPUT);
		pinMode(4,OUTPUT);
		pinMode(7,OUTPUT);
		pinMode(23,OUTPUT);

		//Columnas
		pinMode(14,OUTPUT);
		pinMode(17,OUTPUT);
		pinMode(18,OUTPUT);
		pinMode(22,OUTPUT);

		//Pulsadores
		pinMode(PIN_BOTON_DCHA,INPUT);
		pinMode(PIN_BOTON_IZDA,INPUT);

}

static void refresca_matriz(union sigval arg){
	//DEFINICIÓN DE PINES DE LA RASPI
	int i;

	//Escritura de las filas
	piLock(STD_IO_BUFFER_KEY);
	for (i= 0; i < MATRIZ_ALTO; i++) {
		if ((juego.arkanoPi.pantalla.matriz[columna][i] == 1)) { // ¿o [9-columna]?
			switch (i){
				case 5:
					digitalWrite(7,LOW);
					break;
				case 6:
					digitalWrite(23,LOW);
					break;
				default :
					digitalWrite(i,LOW);
					break;
				}
		}else{
			switch (i){
				case 5:
					digitalWrite(7,HIGH);
					break;
				case 6:
					digitalWrite(23,HIGH);
					break;
				default :
					digitalWrite(i,HIGH);
					break;

			}
		}
	}
	//Escritura en las columnas(número binario)
	digitalWrite(14,columna & 0x01);
	digitalWrite(17,columna & 0x02);
	digitalWrite(18,columna & 0x04);
	digitalWrite(22,columna & 0x08);
	piUnlock(STD_IO_BUFFER_KEY);
	columna++; //Actualizo para que en la siguiente cuenta salte de columna
	if(columna>=10)
		columna = 0;
	tmr_startms(tmr,1);
}

static void refresca_pelota(union sigval arg){
	piLock(FLAGS_KEY);
	flags |= FLAG_PELOTA;
	piUnlock(FLAGS_KEY);
	tmr_startms(tmr_pelota,500);
}

void botonDerecha(void){
	printf("boton derecha\n");

	piLock(FLAGS_KEY);
	flags |= FLAG_TECLA;
	flags |= FLAG_RAQUETA_DERECHA;
	piUnlock(FLAGS_KEY);


}

void botonIzquierda(void){
	printf("boton izquierda\n");

	piLock(FLAGS_KEY);
	flags |= FLAG_TECLA;
	flags |= FLAG_RAQUETA_IZQUIERDA;
	piUnlock(FLAGS_KEY);
}

int main (){
	tmr = tmr_new(refresca_matriz);
    unsigned int next;

    //Maquina de estados: lista de transiciones
    //{EstadoOrigen,FuncionEntrada,EstadoDestino,FuncionSalida}
    fsm_trans_t estado_juego[] = {
            {WAIT_START,CompruebaTeclaPulsada,WAIT_PUSH,InicializaJuego},
            {WAIT_PUSH,CompruebaTeclaPelota,WAIT_PUSH,MovimientoPelota},
            {WAIT_PUSH,CompruebaTeclaRaquetaDerecha,WAIT_PUSH,MueveRaquetaDerecha},
            {WAIT_PUSH,CompruebaTeclaRaquetaIzquierda,WAIT_PUSH,MueveRaquetaIzquierda},
            {WAIT_PUSH,CompruebaFinalJuego,WAIT_END,FinalJuego},
            {WAIT_END,CompruebaTeclaPulsada,WAIT_START,ReseteaJuego},
            {-1,NULL,-1,NULL},
    };

    fsm_t* juego_fsm = fsm_new(WAIT_START,estado_juego,NULL);

    //Configuracion e inicializacion del sistema
    systemSetup();
    fsm_setup(juego_fsm);

    //Inicializacion interrupciones pulsadores
    wiringPiISR(PIN_BOTON_DCHA,INT_EDGE_FALLING,botonDerecha);
    wiringPiISR(PIN_BOTON_IZDA,INT_EDGE_FALLING,botonIzquierda);

    next = millis();
    inicializaPines();
    tmr_startms(tmr,1);
    while(1) {
        fsm_fire(juego_fsm);
        next += CLK_MS;
        delay_until(next);

    }
    fsm_destroy(juego_fsm);
}

