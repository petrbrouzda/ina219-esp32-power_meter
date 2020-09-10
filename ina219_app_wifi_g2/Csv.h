#ifndef X_CSV_H
#define X_CSV_H

class Csv
{
  public:
    Csv( int size, char decimalPoint, const char * delimiter);
    void beginHeader();
    void endHeader();
    void rewind();
    int getUsage();
    int getSize();
    char * getContent();
    void addInt( int i );
    void addDouble( double d );
    void addString( const char * s );
    void endLine();
    bool hasData();
    

  private:
    char * data;
    int size;
    int dataStart;
    int endPtr;
    char decimalPoint;
    const char * delimiter;
    bool firstItem;
    void addField( const char * s );
};

#endif
