#include<RTClib.h>
#include<Wire.h>
#include<Ethernet.h>

#define TEMPO_MOTOR_LIG 20 //Tempo do Motor ligado por grama de ração

//Variáveis
unsigned int qtda_animais;
String tipo_animal;
unsigned float peso_animal;
unsigned float tax_crescimento; //Taxa de crescimento do Animal por dia

unsigned float qtda_racao_diaria;
unsigned float qtda_racao_por_refeicao;
unsigned float qtda_racao;

//Bibliotecas externas
RTC_DS3231 rtc; //Acessar módulo de hórario em tempo real

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
