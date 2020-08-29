#ifndef X_VALUES_H
#define X_VALUES_H

long long addFloat( long long val, float num );
float getVal( long long val );

class Values
{
  public:
    /** cas posledniho predesleho mereni millis() - abychom vedeli, kolik casu uplynulo */
    long lastMeteringTime = 0;

    /** celkova spotrebovana energie */
    long long total_uWh = 0;
    
    long total_ms = 0;

    /** celkovy spotrebovany proud*cas (kapacita) */
    long long total_uAh = 0;
    
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

    /** cas msec v low power */
    long lowPowerTime = 0;
    /** cas msec v high power */
    long highPowerTime = 0;

    /** celkova spotreba v low power */
    long long lowPower_uWh = 0;
    /** celkova spotreba v high power */
    long long highPower_uWh = 0;
    /** celkova spotreba v low power */
    long long lowPower_uAh = 0;
    /** celkova spotreba v high power */
    long long highPower_uAh = 0;

    void doCompute( int coMerim, long interval_ms, long curTime, float lowHighThreshold );
    void resetCounters();
    void resetMinuteData();
};

#endif
