#ifndef X_VALUES_H
#define X_VALUES_H

class Values
{
  public:
    /** cas posledniho predesleho mereni millis() - abychom vedeli, kolik casu uplynulo */
    long lastMeteringTime = 0;
    
    double total_uWh = 0;
    long total_ms = 0;
    double total_uAh = 0;
    
    /** napeti - spotrebice nebo zdroje (za nebo pred odporem) podle toho, co merime */
    float outVoltage;
    /** vykon - spotrebice nebo zdroje (za nebo pred odporem) podle toho, co merime */
    float outPower;
    
    /** napeti na odporu */
    float shuntvoltage;
    /** napeti na spotrebici - za odporem */
    float busvoltage;
    /** proud */
    float current_mA;
    /** vykon na spotrebici */
    float power_mW;
    /** napeti zdroje - pred odporem*/
    float source;
    /** vykon zdroje */
    float source_mW;
    /** vykon na odporu */
    float shunt_power_mW;
    /** pomer (procenta) vykonu na odporu ku vykonu na spotrebici */
    float shunt_pomer;
    
    /** celkova spotrebovana energie */
    double energy_uWh;
    /** prumerny vykon */
    double avg_power;
    
    /** max. proud v teto minute */
    float maxCurrentMin = 0;
    /** max proud celkove */
    float maxCurrentTotal = 0;
    
    /** min napeti celkove */
    float minVoltageTotal = 100;
    /** max napeti celkove */
    float maxVoltageTotal = -100;
    /** min napeti v teto minute */
    float minVoltageMin = 100;
    /** min napeti v teto minute */
    float maxVoltageMin = -100;
    
    /** celkovy mereny cas msec */
    long meteringTime = 0;  

    void doCompute( int coMerim, long interval_ms, long curTime );
    void resetCounters();
    void resetMinuteData();
};

#endif
