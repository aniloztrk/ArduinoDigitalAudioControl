//Anıl Öztürk
//TDA7313 Dijital Ton Kontrol Projesi
//13.06.2023

#include <Wire.h>
#include <EEPROM.h>
#include <LCD5110_Basic.h>
#include <LiquidCrystal.h> 
#include <IRremote.h>
#include <TDA7313.h>
#include <MsTimer2.h>

#define keypad 7

#define buzzer 7

//Kumandamızdan gelen kodları isimlendirdik
#define remote_button_up 0xFF18E7
#define remote_button_down 0xFF4AB5
#define remote_button_left 0xFF10EF
#define remote_button_right 0xFF5AA5
#define remote_button_center 0xFF38C7
#define remote_button_volume_increase 0xFFA25D
#define remote_button_volume_decrease 0xFF22DD
#define remote_button_bass_increase 0xFF629D
#define remote_button_bass_decrease 0xFF02FD
#define remote_button_treble_increase 0xFFE21D
#define remote_button_treble_decrease 0xFFC23D
#define remote_button_loudness_toggle 0xFF6897
#define remote_button_gain_change 0xFFB04F
#define remote_button_input_change 0xFF9867 

unsigned long remote_old_key = 0;

//Ekran, ses işlemcisi ve ir alıcının tanımlamalarını yaptık
LCD5110 myLCD(8, 9, 10, 12, 11);
LiquidCrystal lcd( 7 , 6 , 2 , 3 , 4 , 5 ) ; // RS,E,D4,D5,D6,D7 

TDA7313 tda;

IRrecv irrecv(6);

decode_results results;

//Lcd ekran için gerekli yazı fontları
extern uint8_t SmallFont[];
extern uint8_t MediumNumbers[];
extern uint8_t BigNumbers[];

//Menüler arasında geçiş yapmamızı sağlayan değiken
int menu_pos;

//Ses işlemcimiz için gereken değişkenler
int volume;
int bass;
int treble;
int loudness;
int gain;
int input;
int lr;
int rr;
int lf;
int rf;

//Mute değişkeni ve mute kapandıktan sonra geri aynı sese dönmemizi sağlayan değişken
bool mute;
int old_volume;

//En son yapılan işlemin zamanını tutan değişken
unsigned long last_activity;

//Değişkeni arduinonun o anki çalışma süresine eşitliyoruz
void SetLastActivity() {
  last_activity = millis();
}

bool buzzer_active = false;
bool buzzer_active_date = 0;

void Buzzer(){
  if(!buzzer_active){
    digitalWrite(buzzer, HIGH);
    buzzer_active = true;
  }
  else{
    if(millis() - buzzer_active_date > 40){
      buzzer_active = false;
      digitalWrite(buzzer, LOW);
      MsTimer2::stop();
    }
  }
}

void setup() {

  pinMode(buzzer, OUTPUT);
  pinMode(2, OUTPUT);

  Serial.begin(9600);
  irrecv.enableIRIn();

  Wire.begin();
  myLCD.InitLCD();

  //Rom üzerinden verileri okuyup değişkenlere atıyoruz
  volume = EEPROM.read(0);
  if (volume > 63) volume = 10;

  bass = EEPROM.read(1);
  if (bass > 7) bass = 0;

  treble = EEPROM.read(2);
  if (treble > 7) treble = 0;

  loudness = EEPROM.read(3);
  if (loudness > 1) loudness = 0;

  gain = EEPROM.read(4);
  if (gain > 3) gain = 0;

  input = EEPROM.read(5);
  if (input > 2) input = 0;

  lr = EEPROM.read(6);
  if (lr > 31) lr = 0;

  rr = EEPROM.read(7);
  if (rr > 31) rr = 0;

  lf = EEPROM.read(8);
  if (lf > 31) lf = 0;

  rf = EEPROM.read(9);
  if (rf > 31) rf = 0;

  mute = false;

  SetTda();
  VolumePage(volume);
  MsTimer2::set(8, Buzzer);
}

void loop() {
  //Ir alıcıdan gelen verileri okuyoruz
  if (irrecv.decode(&results)) {
    if(results.value != 0xFFFFFFFF) 
      remote_old_key = results.value;
    ReadRemoteControl();
  }

  //Butonları kontrol ediyoruz
  int keypad_value = analogRead(keypad);
  if (keypad_value <= 985 && keypad_value >= 975) {
    SetLastActivity();
    if (mute) UnMute();
    IncreaseValue();
    delay(150);
  }

  if (keypad_value == 1023 || keypad_value == 1022) {
    SetLastActivity();
    if (mute) UnMute();
    DecreaseValue();
    delay(150);
  }

  if (keypad_value <= 965 && keypad_value >= 955) {
    SetLastActivity();
    PreviousPage();
    delay(300);
  }

  if (keypad_value <= 1005 && keypad_value >= 995) {
    SetLastActivity();
    NextPage();
    delay(300);
  }

  //Belirli süre boyunca hiçbir tuşa basılmazsa ana ekrana dönüyoruz
  if ((millis() - last_activity > 15000) && !mute && menu_pos != 0) {
    menu_pos = 0;
    VolumePage(volume);
  }

  if(input == 1) digitalWrite(2, HIGH);

  else digitalWrite(2, LOW);
}

//Ir alıcıdan gelen verileri işliyoruz
void ReadRemoteControl(){
   switch (results.value) {
      case remote_button_up:
        SetLastActivity();
        results.value = 0;
        IncreaseValue();
        break;

      case remote_button_down:
        SetLastActivity();
        results.value = 0;
        DecreaseValue();
        break;

      case remote_button_left:
        SetLastActivity();
        results.value = 0;
        PreviousPage();
        break;

      case remote_button_right:
        SetLastActivity();
        results.value = 0;
        NextPage();
        break;

      case remote_button_center:
        SetLastActivity();
        results.value = 0;
        if (mute) {
          UnMute();
          delay(250);
        } else {
          Mute();
          delay(2000);
        }
        break;

      case remote_button_volume_increase:
        SetLastActivity();
        if (menu_pos != 0) {
          menu_pos = 0;
          VolumePage(volume);
        }
        IncreaseValue();
        break;

      case remote_button_volume_decrease:
        SetLastActivity();
        if (menu_pos != 0) {
          menu_pos = 0;
          VolumePage(volume);
        }
        DecreaseValue();
        break;

      case remote_button_bass_increase:
        SetLastActivity();
        if (menu_pos != 1) {
          menu_pos = 1;
          TonePage();
          myLCD.setFont(SmallFont);
          myLCD.print("-*-", 11, 0);
        }
        IncreaseValue();
        break;

      case remote_button_bass_decrease:
        SetLastActivity();
        if (menu_pos != 1) {
          menu_pos = 1;
          TonePage();
          myLCD.setFont(SmallFont);
          myLCD.print("-*-", 11, 0);
        }
        DecreaseValue();
        break;

      case remote_button_treble_increase:
        SetLastActivity();
        if (menu_pos != 2) {
          menu_pos = 2;
          TonePage();
          myLCD.setFont(SmallFont);
          myLCD.print("-*-", 54, 0);
        }
        IncreaseValue();
        break;

      case remote_button_treble_decrease:
        SetLastActivity();
        if (menu_pos != 2) {
          menu_pos = 2;
          TonePage();
          myLCD.setFont(SmallFont);
          myLCD.print("-*-", 54, 0);
        }
        DecreaseValue();
        break;

      case remote_button_loudness_toggle:
        SetLastActivity();
        if (menu_pos != 3) {
          menu_pos = 3;
          LoudnessPage();
        }
        IncreaseValue();
        break;
      
      case remote_button_gain_change:
        SetLastActivity();
        if (menu_pos != 4) {
          menu_pos = 4;
          GainPage();
        }
        IncreaseValue();
        break;

      case remote_button_input_change:
        SetLastActivity();
        if (menu_pos != 5) {
          menu_pos = 5;
          InputPage();
        }
        IncreaseValue();
        break;

      //Tuşa basılı tutulırsa 0xf değeri dönüyor ve bir önceki basılan tuşa göre işlem yapıyoruz
      case 0xFFFFFFFF:
        SetLastActivity();
        switch (remote_old_key) {
          case remote_button_up:
            results.value = 0;
            IncreaseValue();
            break;

          case remote_button_down:
            results.value = 0;
            DecreaseValue();
            break;

          case remote_button_volume_increase:
            if (menu_pos != 0) {
              menu_pos = 0;
              VolumePage(volume);
            }
            IncreaseValue();
            break;

          case remote_button_volume_decrease:
            if (menu_pos != 0) {
              menu_pos = 0;
              VolumePage(volume);
            }
            DecreaseValue();
            break;

          case remote_button_bass_increase:
            if (menu_pos != 1) {
              menu_pos = 1;
              TonePage();
              myLCD.setFont(SmallFont);
              myLCD.print("-*-", 11, 0);
            }
            IncreaseValue();
            break;

          case remote_button_bass_decrease:
            if (menu_pos != 1) {
              menu_pos = 1;
              TonePage();
              myLCD.setFont(SmallFont);
              myLCD.print("-*-", 11, 0);
            }
            DecreaseValue();
            break;

          case remote_button_treble_increase:
            if (menu_pos != 2) {
              menu_pos = 2;
              TonePage();
              myLCD.setFont(SmallFont);
              myLCD.print("-*-", 54, 0);
            }
            IncreaseValue();
            break;

          case remote_button_treble_decrease:
            if (menu_pos != 2) {
              menu_pos = 2;
              TonePage();
              myLCD.setFont(SmallFont);
              myLCD.print("-*-", 54, 0);
            }
            DecreaseValue();
            break;
        }
        delay(75);
        break;
    }
    irrecv.resume();
}

//menu_pos değişkenine göre sayfa atlıyoruz
void NextPage() {
  if (mute) {
    UnMute();
    return;
  }

  if(!buzzer_active) MsTimer2::start();

  switch (menu_pos) {
    case 0:
      menu_pos = 1;
      TonePage();
      myLCD.setFont(SmallFont);
      myLCD.print("-*-", 11, 0);
      break;

    case 1:
      menu_pos = 2;
      myLCD.clrRow(0);
      myLCD.setFont(SmallFont);
      myLCD.print("-*-", 54, 0);
      break;

    case 2:
      menu_pos = 3;
      LoudnessPage();
      break;

    case 3:
      menu_pos = 4;
      GainPage();
      break;

    case 4:
      menu_pos = 5;
      InputPage();
      break;

    case 5:
      menu_pos = 6;
      BalancePage();
      myLCD.print("-*-", 2, 0);
      break;

    case 6:
      menu_pos = 7;
      myLCD.clrRow(0);
      myLCD.print("-*-", 22, 0);
      break;

    case 7:
      menu_pos = 8;
      myLCD.clrRow(0);
      myLCD.print("-*-", 42, 0);
      break;

    case 8:
      menu_pos = 9;
      myLCD.clrRow(0);
      myLCD.print("-*-", 61, 0);
      break;

    case 9:
      menu_pos = 0;
      VolumePage(volume);
      break;
  }
}

//menu_pos değişkenine göre sayfa atlıyoruz
void PreviousPage() {
  if (mute) {
    UnMute();
    return;
  }
  if(!buzzer_active) MsTimer2::start();

  switch (menu_pos) {
    case 0:
      menu_pos = 9;
      BalancePage();
      myLCD.print("-*-", 61, 0);
      break;

    case 9:
      menu_pos = 8;
      myLCD.clrRow(0);
      myLCD.print("-*-", 42, 0);
      break;

    case 8:
      menu_pos = 7;
      myLCD.clrRow(0);
      myLCD.print("-*-", 22, 0);
      break;

    case 7:
      menu_pos = 6;
      myLCD.clrRow(0);
      myLCD.print("-*-", 2, 0);
      break;

    case 6:
      menu_pos = 5;
      InputPage();
      break;

    case 5:
      menu_pos = 4;
      GainPage();
      break;

    case 4:
      menu_pos = 3;
      LoudnessPage();
      break;

    case 3:
      menu_pos = 2;
      TonePage();
      myLCD.setFont(SmallFont);
      myLCD.print("-*-", 54, 0);
      break;

    case 2:
      menu_pos = 1;
      myLCD.clrRow(0);
      myLCD.setFont(SmallFont);
      myLCD.print("-*-", 11, 0);
      break;

    case 1:
      menu_pos = 0;
      VolumePage(volume);
      break;
  }
}

//menu_pos değişkenine göre sayfadaki değerleri değiştiriyoruz
void DecreaseValue() {
  if(!buzzer_active) MsTimer2::start();
  switch (menu_pos) {
    case 0:
      if (volume > 0) {
        volume--;
        UpdateVolume(volume);
      }
      break;

    case 1:
      if (bass > -7) {
        bass--;
        UpdateBass();
      }
      break;

    case 2:
      if (treble > -7) {
        treble--;
        UpdateTreble();
      }
      break;

    case 3:
      if (loudness) {
        loudness = false;
      } else {
        loudness = true;
      }
      UpdateLoudness();
      break;

    case 4:
      switch(gain){
        case 3:
          gain = 2; break;
        case 2:
          gain = 1; break;
        case 1:
          gain = 0; break;
        case 0:
          gain = 2; break;
      }
      UpdateGain();
      break;

    case 5:
      switch(input){
        case 2:
          input = 1; break;
        case 1:
          input = 0; break;
        case 0:
          input = 2; break;
      }
      UpdateInput();
      break;

    case 6:
      if (lr > 0) {
        lr--;
        UpdateLr();
      }
      break;

    case 7:
      if (rr > 0) {
        rr--;
        UpdateRr();
      }
      break;

    case 8:
      if (lf > 0) {
        lf--;
        UpdateLf();
      }
      break;

    case 9:
      if (rf > 0) {
        rf--;
        UpdateRf();
      }
      break;
  }
}

//menu_pos değişkenine göre sayfadaki değerleri değiştiriyoruz
void IncreaseValue() {
  if(!buzzer_active) MsTimer2::start();
  switch (menu_pos) {
    case 0:
      if (volume < 63) {
        volume++;
        UpdateVolume(volume);
      }
      break;

    case 1:
      if (bass < 7) {
        bass++;
        UpdateBass();
      }
      break;

    case 2:
      if (treble < 7) {
        treble++;
        UpdateTreble();
      }
      break;

    case 3:
      if (loudness) {
        loudness = false;
      } else {
        loudness = true;
      }
      UpdateLoudness();
      break;

    case 4:
      switch(gain){
        case 0:
          gain = 1; break;
        case 1:
          gain = 2; break;
        case 2: 
          gain = 3; break;
        case 3:
          gain = 0; break;
      }
      UpdateGain();
      break;

    case 5:
      switch(input){
        case 0:
          input = 1; break;
        case 1:
          input = 2; break;
        case 2: 
          input = 0; break;
      }
      UpdateInput();
      break;

    case 6:
      if (lr < 31) {
        lr++;
        UpdateLr();
      }
      break;

    case 7:
      if (rr < 31) {
        rr++;
        UpdateRr();
      }
      break;

    case 8:
      if (lf < 31) {
        lf++;
        UpdateLf();
      }
      break;

    case 9:
      if (rf < 31) {
        rf++;
        UpdateRf();
      }
      break;
  }
}

//Sistemi sessize alıyoruz
void Mute() {
  if(!buzzer_active) MsTimer2::start();
  mute = true;
  menu_pos = 0;
  old_volume = volume;
  tda.setVolume(0);

  ClearLCD();
  myLCD.setFont(SmallFont);
  myLCD.print("--------------", CENTER, 0);
  myLCD.print("Ses Seviyesi", CENTER, 8);
  myLCD.print("SESSIZ", CENTER, 24);
  myLCD.print("--------------", CENTER, 40);
}

//Sistemin sesini açıyoruz
void UnMute() {
  if(!buzzer_active) MsTimer2::start();
  mute = false;
  menu_pos = 0;
  VolumePage(old_volume);
}

//Sayfaları ekrana yazdırıyoruz
void VolumePage(int vol) {
  ClearLCD();

  myLCD.setFont(SmallFont);
  myLCD.print("--------------", CENTER, 0);
  myLCD.print("Ses Seviyesi", CENTER, 8);

  UpdateVolume(vol);

  myLCD.setFont(SmallFont);
  myLCD.print("--------------", CENTER, 40);
}

void TonePage() {
  ClearLCD();

  myLCD.setFont(SmallFont);
  myLCD.print("Bas", 11, 8);
  myLCD.print("Tiz", 54, 8);

  myLCD.setFont(MediumNumbers);
  UpdateBass();
  UpdateTreble();

  myLCD.setFont(SmallFont);
  myLCD.print("--------------", CENTER, 40);
}

void LoudnessPage() {
  ClearLCD();

  myLCD.setFont(SmallFont);
  myLCD.print("--------------", CENTER, 0);
  myLCD.print("Loudness", CENTER, 8);

  UpdateLoudness();

  myLCD.setFont(SmallFont);
  myLCD.print("--------------", CENTER, 40);
}

void GainPage() {
  ClearLCD();

  myLCD.setFont(SmallFont);
  myLCD.print("--------------", CENTER, 0);
  myLCD.print("Kazanc", CENTER, 8);

  UpdateGain();

  myLCD.setFont(SmallFont);
  myLCD.print("--------------", CENTER, 40);
}

void InputPage() {
  ClearLCD();

  myLCD.setFont(SmallFont);
  myLCD.print("--------------", CENTER, 0);
  myLCD.print("Giris", CENTER, 8);

  UpdateInput();

  myLCD.setFont(SmallFont);
  myLCD.print("--------------", CENTER, 40);
}

void BalancePage() {
  ClearLCD();

  myLCD.setFont(SmallFont);

  myLCD.print("LR", 5, 8);
  UpdateLr();

  myLCD.print("RR", 25, 8);
  UpdateRr();

  myLCD.print("LF", 45, 8);
  UpdateLf();

  myLCD.print("RF", 65, 8);
  UpdateRf();

  myLCD.print("--------------", CENTER, 40);
}

//Değerleri ekranda ve ses işlemcisinin içinde güncelliyoruz
void UpdateVolume(int vol) {
  tda.setVolume(vol);

  EEPROM.update(0, vol);

  myLCD.clrRow(2);
  myLCD.clrRow(3);
  myLCD.clrRow(4);
  myLCD.setFont(MediumNumbers);
  myLCD.printNumI(vol, CENTER, 24);
}

void UpdateBass() {
  tda.setBass(bass);

  EEPROM.update(1, bass);

  myLCD.clrRow(2, 0, 45);
  myLCD.clrRow(3, 0, 45);
  myLCD.clrRow(4, 0, 45);
  myLCD.setFont(MediumNumbers);

  if (bass < 0) {
    myLCD.printNumI(bass, 5, 24);
  } else {
    myLCD.printNumI(bass, 15, 24);
  }
}

void UpdateTreble() {
  tda.setTreble(treble);

  EEPROM.update(2, treble);

  myLCD.clrRow(2, 45);
  myLCD.clrRow(3, 45);
  myLCD.clrRow(4, 45);
  myLCD.setFont(MediumNumbers);

  if (treble < 0) {
    myLCD.printNumI(treble, 48, 24);
  } else {
    myLCD.printNumI(treble, 58, 24);
  }
}

void UpdateLoudness() {
  tda.setSwitch(input, loudness, gain);

  EEPROM.update(3, loudness);

  myLCD.clrRow(3);
  if (loudness) {
    myLCD.print("Aktif", CENTER, 24);
  } else {
    myLCD.print("Pasif", CENTER, 24);
  }
}

void UpdateGain() {
  tda.setSwitch(input, loudness, gain);

  EEPROM.update(4, gain);

  myLCD.clrRow(3);
  switch (gain) {
    case 0:
      myLCD.print("0.00 dB", CENTER, 24);
      break;

    case 1:
      myLCD.print("3.75 dB", CENTER, 24);
      break;

    case 2:
      myLCD.print("7.50 dB", CENTER, 24);
      break;

    case 3:
      myLCD.print("11.25 dB", CENTER, 24);
      break;
  }
}

void UpdateInput() {
  tda.setSwitch(input, loudness, gain);

  EEPROM.update(5, input);

  myLCD.clrRow(3);
  myLCD.printNumI(input + 1, CENTER, 24);
}

void UpdateLr() {
  tda.setAttLR(lr);

  EEPROM.update(6, lr);

  myLCD.clrRow(3, 5, 24);
  if (lr >= 10) {
    myLCD.printNumI(lr, 5, 24);
  } else {
    myLCD.printNumI(lr, 8, 24);
  }
}

void UpdateRr() {
  tda.setAttRR(rr);

  EEPROM.update(7, rr);

  myLCD.clrRow(3, 24, 44);
  if (rr >= 10) {
    myLCD.printNumI(rr, 25, 24);
  } else {
    myLCD.printNumI(rr, 28, 24);
  }
}

void UpdateLf() {
  tda.setAttLF(lf);

  EEPROM.update(8, lf);

  myLCD.clrRow(3, 44, 64);
  if (lf >= 10) {
    myLCD.printNumI(lf, 45, 24);
  } else {
    myLCD.printNumI(lf, 48, 24);
  }
}

void UpdateRf() {
  tda.setAttRF(rf);

  EEPROM.update(9, rf);

  myLCD.clrRow(3, 64);
  if (rf >= 10) {
    myLCD.printNumI(rf, 65, 24);
  } else {
    myLCD.printNumI(rf, 68, 24);
  }
}

//Başta atadığımız deşikenlerle ses işlemcisini güncelliyoruz
void SetTda() {
  tda.setVolume(volume); //0 ile 63 arasında değer alır
  tda.setBass(bass); //-7 ile 7 arasında değer alır
  tda.setTreble(treble); //-7 ile 7 arasında değer alır
  tda.setSwitch(input, loudness, gain); //input 0 - 1 - 2 olarak 3 değer alır, loudness 0 - 1 olarak 2 değer alır, gain 0 - 1 - 2 olarak 3 değer alır
  tda.setAttLR(lr); // 0 ile 31 arasında değer alır
  tda.setAttRR(rr); // 0 ile 31 arasında değer alır
  tda.setAttLF(lf); // 0 ile 31 arasında değer alır
  tda.setAttRF(rf); // 0 ile 31 arasında değer alır, 31 en kısık sestir
}

//Ekranımızı tamamen siliyoruz
void ClearLCD() {
  myLCD.clrRow(0);
  myLCD.clrRow(1);
  myLCD.clrRow(2);
  myLCD.clrRow(3);
  myLCD.clrRow(4);
  myLCD.clrRow(5);
  myLCD.clrRow(6);
  myLCD.clrRow(7);
}