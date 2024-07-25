/*
  Program ini program V8 mesin washer dan filling galon.
  dibuat oleh Thomas Wismoyo
  program ini menggunakan input pullup (normally 5v)
  pengkabelan input dihubungkan ke ground untuk ON (agar 0v)
  11 November 2023
  -----------------
  Note dari V7:
  1. bug di if else bikin ifnya independen
  2. ada bug valuenya gak bener
  3. acuan aktuatornya based on filling semua
  4. 
  5. 
*/

#include <AccelStepper.h>
//------Variabel Setting--------------
//kalau waktu pakai unsigned long
int kecepatanWasher = 800;      //kecepatan stepper washer
int percepatanWasher = 1000;    //percepatan stepper washer
int kecepatanStopper = 800;     //kecepatan stepper washer
int percepatanStopper = 1000;   //percepatan stepper washer
int lWasher = 5130;             //langkah untuk setiap pergeseran washer
int sWasher = 150;              //langkah untuk geser washer sedikit
int onStopper = 500;            //Posisi stopper mengganjal
unsigned long washTime = 2000;  //lamanya pencucian galon
unsigned long befFillTime = 2000; //timer agar dua galon dempet
//(befFillTime befFillPV befFillFV)
unsigned long fillTime1 = 2000;  //lamanya pengisian galon
unsigned long fillTime2 = 2000;  //lamanya pengisian galon
//(fillTime fillPV fillFV)
unsigned long aftFillTime = 1000; //timer setelah isi untuk stop arm
//(aftFillTime aftFillPV aftFillFV)

//-----------PIN ASSIGNMENT------------
// Pin 22 Free
#define pinDirPress 23  //OUTPUT DC MOTOR
#define pinSigPress 24  //OUTPUT DC MOTOR
#define pinEnblStopper 25     //OUTPUT STEPPER
#define pinStepStopper 26   //OUTPUT STEPPER
#define pinDirStopper 27    //OUTPUT STEPPER
#define pinEnblWasher 28      //OUTPUT STEPPER
#define pinStepWasher 29      //OUTPUT STEPPER
#define pinDirWasher 30       //OUTPUT STEPPER
#define pinBArdAuto 31     //INPUT
#define pinBDirWasher 32   //INPUT
#define pinBStepWasher 33  //INPUT
#define pinBTopPress 34    //INPUT
#define pinBBotPress 35    //INPUT
#define pinIRWasher 36        //INPUT Washer
#define pinIRFilling 37       //INPUT Filling
#define pinIRPress 38      //INPUT PressTutup
// Pin 39 Free OUTPUT RELAY
#define pinFilling 40   //OUTPUT RELAY
#define pinConvFill 41      //OUTPUT RELAY
#define pinColdPump 42  //OUTPUT RELAY
#define pinHotPump 43   //OUTPUT RELAY

//----------Variabel Operasional-------
int f1 = 0;                                  //variabel untuk pengulangan for level 1.
bool ember[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };  //isi washer 0-7 total ada 8
#define nyala 1                              //
#define mati 0                               //
int adaGalonWash = 0;
int adaGalonFill = 0;
#define pWasher 7                             //Panjang bucket, berubah untuk trouble shoot.
int wState = 0;                               //Status Washer sedang cuci atau tidak
unsigned long targetHot;  //pembantu timer cucian panas. milis+washtime.
unsigned long targetCold; //pembantu timer pencucian dingin
unsigned long befFillPV = 0; //present value sebelum isi galon.
unsigned long fillPV1 = 0; //present value isi galon
unsigned long fillPV2 = 0; //present value air sisaan galon
unsigned long aftFillPV = 0; //present value setelah isi galon
bool hotPump = 0;   //Status hot pump
bool coldPump = 0;  //Status cold pump
bool filling = 0; //Status filling pump
int galonCtr = 0; //counter galon filling
bool befFillFV = 0; //status timer sebelum pengisian
bool fillFV1 = 0; //selesai pengisian
bool fillFV2 = 0; //selesai air sisaan
bool aftFillFV = 0; //status timer setelah pengisian
bool convFilling = 0; //status conveyor filling
bool PS_Pump = 1; //Previous State untuk mencegah double input adaGalonFill
bool PS_IRWash = 1;
bool PS_IRFill = 1; // Previous State untuk mencegah double input IR Filling
bool STA = 0; //Status Stop Arm. HIGH = mengganjal
int langkah = 0; //tektokan timer


//------Variabel Pemrograman----------
AccelStepper konvCuci(AccelStepper::DRIVER, pinStepWasher, pinDirWasher);   //penggerak conveyor mesin cuci
AccelStepper stopArm(AccelStepper::DRIVER, pinStepStopper, pinDirStopper);  //penggerak stopper filling galon

//-----Daftar fungsi yang ada--------
void arduinoAuto();
void manual();

void setup() {
  //definisikan output modul. 1 sama dengan nyala
  for (f1 = 22; f1 <= 30; f1++) {
    pinMode(f1, OUTPUT);
    digitalWrite(f1, mati); //nol sama dengan mati
  }
  digitalWrite(pinEnblStopper, 1);
  //definisikan input tombol dan sensor 1 SAMA DENGAN MATI
  for (f1 = 31; f1 <= 37; f1++) {
    pinMode(f1, INPUT_PULLUP);
  }

  //definisikan output relay. 1 sama dengan nyala
  for (f1 = 38; f1 <= 43; f1++) {
    pinMode(f1, OUTPUT);
    digitalWrite(f1, mati);
  }

  //Serial Communication
  Serial.begin(9600);

  //definisikan stepper
  konvCuci.setMaxSpeed(kecepatanWasher);
  konvCuci.setAcceleration(percepatanWasher);
  stopArm.setMaxSpeed(kecepatanStopper);
  stopArm.setAcceleration(percepatanStopper);
}

//-----------------VOID LOOP DI SINI-----------------
void loop() {
  while (digitalRead(pinBArdAuto) == 0) {
    arduinoAuto();
  }
  //Program manual
  while (digitalRead(pinBArdAuto) == 1) {
    manual();
  }
}
//-----------------END OF VOID LOOP-----------------
//---------Manual Movement Program--------------
void manual() {
  Serial.println("Masuk Fungsi Manual");
  if (digitalRead(pinBStepWasher) == 0) {
    if (digitalRead(pinBDirWasher) == 0) {
      Serial.println("majuConv");
      digitalWrite(pinEnblWasher, 1);
      konvCuci.enableOutputs();
      konvCuci.move(sWasher);  //
      konvCuci.runToPosition();
      konvCuci.stop();
      konvCuci.disableOutputs();
      digitalWrite(pinEnblWasher, 0);
    } else {
      Serial.println("mundurConv");
      digitalWrite(pinEnblWasher, 1);
      konvCuci.enableOutputs();
      konvCuci.move(-sWasher);  //
      konvCuci.runToPosition();
      konvCuci.stop();
      konvCuci.disableOutputs();
      digitalWrite(pinEnblWasher, 0);
    }
  } else {
    konvCuci.disableOutputs();
  }
}
//--------End manual Movement Program------------

//---------arduinoAuto DI SINI--------------ArduinoAuto DI SINI---------
void arduinoAuto() {
  // TUTUPAN PROSES DEVELOPMENT FILLING
  //--------------PROGRAM WASHER----------------PROGRAM WASHER-----------
  // step1 deteksi galon di input
  if (digitalRead(pinIRWasher) == 0 && PS_IRWash == LOW) { //0 pada sensor NPN artinya aktif
    ember[0] = 1;
    PS_IRWash = HIGH; //agar tidak terdeteksi on lagi di signal yang sama
    Serial.println("IR_Wash Nyala");
  }
  if (digitalRead(pinIRWasher) == 1 && PS_IRWash == HIGH) {
    PS_IRWash = LOW; //LOW berarti sudah tidak mendeteksi, dan siap mendeteksi lagi
  }

  //periksa galon di tengah sampai posisi 8
  for (f1 = 0; f1 < pWasher; f1++) { //pWasher panjang washer, default 7
    if (ember[f1] > 0) {
      adaGalonWash ++;
    }
  }

  //-----program cuci air panas----
  if (ember[2] == HIGH) {
    if (hotPump == LOW) {
      targetHot = millis() + washTime;
      digitalWrite(pinHotPump, HIGH);
      hotPump = HIGH;
    }
    if (millis() >= targetHot) {
      digitalWrite(pinHotPump, LOW);
      hotPump = LOW;
    }
  }

  //---program cuci air dingin----
  if (ember[4] == HIGH) {
    if (coldPump == LOW) {
      targetCold = millis() + washTime;
      digitalWrite(pinColdPump, HIGH);
      coldPump = HIGH;
    }
    if (millis() >= targetCold) {
      digitalWrite(pinColdPump, LOW);
      coldPump = LOW;
    }
  }

  //Maju Conveyor kalau ada galon di washer
  if (adaGalonWash > 0 && hotPump == LOW && coldPump == LOW) {
    digitalWrite(pinEnblWasher, HIGH);
    konvCuci.enableOutputs();
    konvCuci.move(-lWasher);  //
    konvCuci.runToPosition();
    konvCuci.stop();
    konvCuci.disableOutputs();
    digitalWrite(pinEnblWasher, LOW);

    //-------GESER BIT EMBER---------
    Serial.println("245 Pantau Ember ");
    for (f1 = pWasher; f1 >= 1; f1 < f1--) {
      ember[f1] = ember[f1 - 1];
      Serial.print(f1);
      Serial.print(" ");
      Serial.println(ember[f1]);
    }

    //-----RESET WASHER STATE------
    ember[0] = 0;
    adaGalonWash = 0;
  }

  //-------------END OF PROGRAM WASHER----------------
  //-------------PROGRAM FILLING----------------------

  //------Periksa Input dari washer---------
  if (coldPump == HIGH && PS_Pump == LOW) {
    adaGalonFill = adaGalonFill + 1;
    PS_Pump = HIGH;
  }
  if (coldPump == LOW && PS_Pump == HIGH) {
    PS_Pump = LOW;
  }

  //hitung galon di stopper
  if (digitalRead(pinIRFilling) == LOW && PS_IRFill == LOW && adaGalonFill > 0) {
    galonCtr = galonCtr + 1;
    PS_IRFill = HIGH;
    Serial.print("galonCtr : ");
    Serial.println(galonCtr);
  }
  if (digitalRead(pinIRFilling) == HIGH && PS_IRFill == HIGH) {
    PS_IRFill = LOW;
  }

  //--------nyalain dan matikan conveyor---------
  //conv nyala kalau filling lagi off dan setelah timer kasih signal
  if (filling == LOW && adaGalonFill > 0 && convFilling == LOW) {
    digitalWrite(pinConvFill, HIGH); //CONVEYOR NYALA
    convFilling = HIGH;
    Serial.println("Conveyor filling High");
  }
  if (filling == HIGH && convFilling == HIGH) { //matiin conveyor kalau masih nyala
    digitalWrite(pinConvFill, LOW);
    convFilling = LOW;
    Serial.println("Conveyor Low");
  }

  //buka dan simpan stopArm. persyaratan untuk simpan stop arm
  if (filling == HIGH && aftFillFV == 0) {
    if (STA == HIGH) {
      Serial.println("stopArm simpan");
      digitalWrite(pinEnblStopper, nyala);
      stopArm.enableOutputs();
      stopArm.move(-onStopper);  //
      stopArm.runToPosition();
      stopArm.stop();
      STA = LOW; //low artinya simpan
    }
  }
  if (STA == LOW && aftFillFV == 1 && adaGalonFill > 0) {
    Serial.println("stopArm maju");
    digitalWrite(pinEnblStopper, nyala);
    stopArm.enableOutputs();
    stopArm.move(onStopper);  //
    stopArm.runToPosition();
    stopArm.stop();
    STA = HIGH; //high artinya maju
  }


  //nyala timer ON filling
  if (galonCtr >= 2 && filling == LOW && langkah == 0) {
    befFillPV = millis() + befFillTime;
    langkah = langkah + 1; //langkah pertama
    Serial.println("Tunggu galon rapet");
  }
  if (millis() > befFillPV && langkah == 1) {
    befFillFV = 1;
    befFillPV = 0;
    langkah = langkah + 1; //langkah ke2
    Serial.println("Galon udah rapet");
  }
  //filling dimulai
  if (befFillFV == 1 && langkah == 2) {
    befFillFV = 0;
    filling = HIGH;
    digitalWrite(pinFilling, HIGH);
    fillPV1 = millis() + fillTime1;
    langkah = langkah + 1; //langkah ke3
    Serial.println("Mulai isi");
  }
  if (millis() > fillPV1 &&  langkah == 3) {
    Serial.println("Pengisian selesai");
    digitalWrite(pinFilling, LOW);
    fillFV1 = 1;
    fillPV1 = 0;
    langkah = langkah + 1; //langkah ke4
  }
  //tunggu air sisaan filling
  if (fillFV1 == 1 && langkah == 4) {
    fillFV1 = 0;
    fillPV2 = millis() + fillTime2;
    langkah = langkah + 1; //langkah ke5
    Serial.println("Tunggu air sisaan");
  }
  if (millis() > fillPV2 && langkah == 5) {
    fillPV2 = 0;
    fillFV2 = 1;
    langkah = langkah + 1; //langkah ke6
    Serial.println("tunggu air sisa selesai");
  }
  if (fillFV2 == 1 && langkah == 6) {
    fillFV2 = 0;
    aftFillPV = millis() + aftFillTime;
    langkah = langkah + 1; //langkah ke7
    adaGalonFill -= 1;
    filling = 0;
    Serial.println("jalan conv tunggu galon lewat");
  }
  if (millis() > aftFillPV && langkah == 7) {
    aftFillPV = 0;
    aftFillFV = 1;
    langkah = 0;
    galonCtr = galonCtr - 2;
    adaGalonFill = adaGalonFill - 1;
    Serial.println("lewat galon selesai");
    Serial.print("galonCtr : ");
    Serial.println(galonCtr);
    Serial.print("ada galon fill : ");
    Serial.println(adaGalonFill);
    Serial.println("loop selesai");
    Serial.println("");
  }
  //---------END OF PROGRAM FILLING--------- END OF PROGRAM FILLING-----
}
//---------arduinoAuto DI ATAS------------------------DI ATAS-------------
