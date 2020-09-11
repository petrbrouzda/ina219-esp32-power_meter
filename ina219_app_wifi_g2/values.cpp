#include "values.h"
#include <arduino.h>


void Values::doCompute( int coMerim, long interval_ms, long curTime, float lowHighThreshold, Csv * csv, bool saveEvents )
{
  this->source = this->busvoltage + (this->shuntvoltage / 1000.0);
  this->source_mW = this->source * this->current_mA;

  if( coMerim==11 ) {
    this->outVoltage = this->busvoltage;
    this->outPower = this->power_mW;
  } else {
    this->outVoltage = this->source;
    this->outPower = this->source_mW;
  }

  this->shunt_power_mW = this->current_mA * this->shuntvoltage / 1000.0;
  this->shunt_pomer = this->power_mW>0 ? (this->shunt_power_mW/this->power_mW*100.0) : 0;

  this->lastMeteringTime = curTime;

  double energy_uWh = this->outPower * ((double)interval_ms) / ((double)3600); 
  this->total_uWh += energy_uWh;
  this->total_ms += interval_ms;
  this->avg_power = this->total_uWh * 3600.0 / ((double)this->total_ms);
  double capa_uAh = this->current_mA * ((double)interval_ms) / ((double)3600); 
  this->total_uAh += capa_uAh ;

  if( this->current_mA > lowHighThreshold ) {
    
    highPowerTime += interval_ms;
    this->highPower_uWh += energy_uWh;
    this->highPower_uAh += capa_uAh ;

    if( this->currentPowerState!=2 ) {
      // zmena stavu low->high
      this->currentPowerState = 2;
      this->lastHighPowerEventStartMsec = curTime;
      this->highPowerEvents++;
      this->maxCurrentHPE = this->current_mA;
      if( saveEvents ) { this->write( csv, "HP" ); }
    }

    if( this->maxCurrentHPE < this->current_mA ) {
      this->maxCurrentHPE = this->current_mA;
    }
    
  } else {
    
    lowPowerTime += interval_ms;
    this->lowPower_uWh += energy_uWh ;
    this->lowPower_uAh += capa_uAh ;
    
    if( this->currentPowerState!=1 ) {
      // zmena stavu high->low
      this->currentPowerState = 1;
      this->lastHighPowerEventLengthMsec = curTime - this->lastHighPowerEventStartMsec;
      if( this->lastHighPowerEventLengthMsec > this->maxHighPowerEventLengthMsec ) {
        this->maxHighPowerEventLengthMsec = this->lastHighPowerEventLengthMsec;
      }
      this->maxCurrentLPE = this->current_mA;
      if( saveEvents ) { this->write( csv, "LP" ); }
    }    

    if( this->maxCurrentLPE < this->current_mA ) {
      this->maxCurrentLPE = this->current_mA;
    }
  }

  this->meteringTime += interval_ms;
  
  if( this->current_mA > this->maxCurrentMin ) {
    this->maxCurrentMin = this->current_mA;
  }
  if( this->current_mA > this->maxCurrentTotal ) {
    this->maxCurrentTotal = this->current_mA;
    this->maxCurrentTime = time(NULL);
  }
  if( this->outVoltage > this->maxVoltageTotal ) {
    this->maxVoltageTotal = this->outVoltage;
    this->maxVoltageTime = time(NULL);
  }
  if( this->outVoltage > this->maxVoltageMin ) {
    this->maxVoltageMin = this->outVoltage;
  }
  if( this->outVoltage < this->minVoltageTotal ) {
    this->minVoltageTotal = this->outVoltage;
    this->minVoltageTime = time(NULL);
  }
  if( this->outVoltage < this->minVoltageMin ) {
    this->minVoltageMin = this->outVoltage;
  }
}

void Values::write( Csv * csv, const char * event )
{
  csv->addDouble( ((double)this->meteringTime) / 1000.0 );
  csv->addString( event );
 
  csv->addDouble( this->outVoltage );
  csv->addDouble( this->current_mA );
  csv->addDouble( this->outPower );
  csv->addDouble( this->total_uAh/1000.0 );
  csv->addString(" ");
  
  csv->addDouble( this->minVoltageMin );
  csv->addDouble( this->maxVoltageMin );
  csv->addDouble( this->maxCurrentMin );
  csv->addDouble( this->minVoltageTotal );
  csv->addDouble( this->maxVoltageTotal );
  csv->addDouble( this->maxCurrentTotal );
  csv->addString(" ");
  
  csv->addInt( this->highPowerEvents );
  csv->addDouble( ((double)this->highPowerTime)/1000.0 );
  csv->addDouble( this->highPower_uAh/1000.0 );
  csv->addDouble( this->highPower_uWh/1000.0 );
  csv->addInt( this->lastHighPowerEventLengthMsec );
  if( this->highPowerEvents>0 ) {
    csv->addInt( (int) ((double)this->highPowerTime)/((double)this->highPowerEvents) );
  } else {
    csv->addString(" " );
  }
  csv->addInt( this->maxHighPowerEventLengthMsec );
  if( strcmp(event,"LP")==0 ) {
    csv->addDouble( this->maxCurrentHPE );  
  } else {
    csv->addString(" ");
  }
  csv->addString(" ");
  
  csv->addDouble( ((double)this->lowPowerTime)/1000.0 );
  csv->addDouble( this->lowPower_uAh/1000.0 );
  csv->addDouble( this->lowPower_uWh/1000.0 );
  if( strcmp(event,"HP")==0 ) {
    csv->addDouble( this->maxCurrentLPE );
  } else {
    csv->addString(" ");
  }
  
  csv->endLine();
}

void Values::writeHeader( Csv * csv  )
{
  csv->addString( "t [s]" );
  csv->addString("event");
  csv->addString( "U [V]" );
  csv->addString( "I [mA]" );
  csv->addString( "P [mW]" );
  csv->addString( "C [mAh]" );
  csv->addString(" ");
  
  csv->addString( "Umin/min [V]" );
  csv->addString( "Umax/min [V]" );
  csv->addString( "Imax/min [mA]" );
  csv->addString( "Umin/total [V]" );
  csv->addString( "Umax/total [V]" );
  csv->addString( "Imax/total [mA]" );
  csv->addString(" ");

  csv->addString( "# of HPE" );
  csv->addString( "HP time [s]" );
  csv->addString( "HP C [mAh]" );
  csv->addString( "HP E [mWh]" );
  csv->addString( "last HP length [ms]" );
  csv->addString( "avg HP length [ms]" );
  csv->addString( "max HP length [ms]" );
  csv->addString( "last HP Imax [mA]" );
  csv->addString(" ");
  
  csv->addString( "LP time [s]" );
  csv->addString( "LP C [mAh]" );
  csv->addString( "LP E [mWh]" );
  csv->addString( "last LP Imax [mA]" );

  csv->endLine();
}


void Values::resetCounters()
{
  this->total_uWh = 0;
  this->total_ms = 0;
  this->total_uAh = 0;    
  this->maxCurrentTotal = 0;
  this->meteringTime = 0;
  this->minVoltageTotal = 100;
  this->maxVoltageTotal = -100;
  this->lowPowerTime = 0;
  this->highPowerTime = 0;
  this->lowPower_uAh = 0;
  this->highPower_uAh = 0;
  this->highPower_uWh = 0;
  this->lowPower_uWh = 0;
  this->currentPowerState = 1;
  this->highPowerEvents = 0;
  this->lastHighPowerEventLengthMsec = 0;
  this->lastHighPowerEventStartMsec = millis();
  this->maxHighPowerEventLengthMsec = 0;
  
  this->resetMinuteData();
}

void Values::resetMinuteData()
{
  this->maxCurrentMin = this->current_mA;
  this->minVoltageMin = this->outVoltage;
  this->maxVoltageMin = this->outVoltage;
  this->minVoltageTime = time(NULL);
  this->maxVoltageTime = time(NULL);
  this->maxCurrentTime = time(NULL);
}
