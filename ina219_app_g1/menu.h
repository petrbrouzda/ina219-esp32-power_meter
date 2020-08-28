#ifndef X_MENU_H
#define X_MENU_H

#define MAX_MENU_ITEMS 20
#define MAX_ITEM_LEN 32 

class Menu
{
  public:
    Menu( const char * nadpis );
    char * nadpis;
    void addItem( const char * text, int code );
    void updateText( int pos, const char * text );
    void setPos( int pos );
    void setActive( int code );
    void clearState();
    int getResult();
    char items[MAX_MENU_ITEMS][MAX_ITEM_LEN];
    int codes[MAX_MENU_ITEMS];
    int curPos;
    int ct; 
    
    /**
     * -1 - zvoleno back 
     * 0 - nezvoleno nic
     * >0 - zvoleny code
     */
    int state;
    bool redraw;
    int active;
};

#endif
