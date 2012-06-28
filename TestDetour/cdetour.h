#ifndef CDETOUR_H
#define CDETOUR_H

class CDetour
{
public:
        void DrawError_H( int, const char *, va_list );

        static void (CDetour::* DrawError_T)( int, const char *, va_list );
};

void (CDetour::* CDetour::DrawError_T)( int, const char *, va_list ) = NULL;

#endif