#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Csv.h"

/*
 *   private:
    char * data;
    int size;
    int dataStart;
    int endPtr;
 */

Csv::Csv( int size, char decimalPoint, const  char * delimiter )
{
    this->decimalPoint = decimalPoint;
    this->delimiter = delimiter;
    this->size = size;
    this->data = (char*)malloc( size + 10 );
    this->endPtr = 0;
    this->dataStart = 0;
    this->firstItem = true;
    this->data[ this->endPtr ] = 0;
}

void Csv::addField( const char * s )
{
  int s1 = strlen( s );
  int s2 = strlen( this->delimiter );
  
  if( this->endPtr > (this->size - s1 - s2) ) return;
  
  if( ! this->firstItem ) {
    // pridat delimiter
    memcpy( this->data + this->endPtr, this->delimiter, s2+1 );
    this->endPtr += s2;
  } else {
    this->firstItem = false;
  }
  memcpy( this->data + this->endPtr, s, s1+1 );
  this->endPtr += s1;
}

void Csv::addInt( int i )
{
  char buff[128];
  sprintf( buff, "%d", i );
  this->addField( buff );
}

void Csv::addDouble( double d )
{
  char buff[128];
  sprintf( buff, "%f", d );
  if( this->decimalPoint != '.' ) {
    char * p = strchr( buff, '.' );
    if( p!=NULL ) {
      *p = this->decimalPoint;
    }
  }
  this->addField( buff );
}

void Csv::addString( const char * s )
{
  this->addField( s );
}

void Csv::endLine() {
  if( this->endPtr > (this->size-2) ) return;
  
  strcpy( this->data + this->endPtr, "\r\n" );
  this->endPtr += 2;
  this->firstItem = true;
}

void Csv::beginHeader()
{
  this->endPtr = 0;
  this->dataStart = 0;
  this->data[ this->endPtr ] = 0;
}

void Csv::endHeader()
{
  this->dataStart = this->endPtr;
}

void Csv::rewind()
{
   this->endPtr = this->dataStart;
   this->data[ this->endPtr ] = 0;
}

int Csv::getUsage()
{
  return (this->endPtr * 100 / this->size) + 1;
}

int Csv::getSize()
{
  return this->endPtr;
}

char * Csv::getContent()
{
   return this->data;
}

bool Csv::hasData()
{
  return this->endPtr != this->dataStart;
}
