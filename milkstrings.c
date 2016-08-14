//============================================================================
// Name        : milkstrings.c
// Author      : Aad van der Geest
// Version     : 0
// Copyright   : 
// Description : Easy strings in c limited length and lifetime
//============================================================================



#include <stdio.h>
#include <stddef.h>
#include <malloc.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

typedef  char * tXt ;


//choose the following numbers wisely depending on available memory
#ifndef txtPOOLSIZE
#define txtPOOLSIZE 32768 // available memory for strings
#define txtMAXLEN 256   // max string length
#endif
#define txtNOTFOUND 9999


#define ZeroMemory(x,y)  memset (x, 0, y)
char nullchar = 0 ;
#define txtEmpty &nullchar

// limit an integer
int lim(int x,int mn,int mx) {
  if (x > mx)
    x = mx ;
  if (x < mn)
    x = mn ;
  return x ; }

// smallest of two integers 
int mini(int x,  int y){
  if (x < y)
    return x;
  else
    return y; }

// all string are allocated in this memory pool
char txtpool[txtPOOLSIZE] ;
int txtpoolidx = 0 ;

//fix the poolidx
void txtFixpool(void) {
  txtpool[txtpoolidx++] = 0 ;
  if (txtpoolidx > txtPOOLSIZE-txtMAXLEN-1) {
    txtpoolidx = 0 ; }
}

//substring
tXt txtSub(tXt tx,int bpos,int len) {
  tXt rslt = &txtpool[txtpoolidx] ;
  int ln = strlen(tx) ;
  int n ;
  if (bpos < 0)
    bpos = lim(ln+bpos-1,0,ln-1) ;
  n = lim(strlen(tx) -bpos,0,len) ;
  if (n > 0)
    memcpy(&txtpool[txtpoolidx],&tx[bpos],n) ;
  txtpoolidx += n ;
  txtFixpool() ;
  return rslt ; }

tXt txtDelete(tXt tx,int bpos,int len) {
  tXt rslt = &txtpool[txtpoolidx] ;
  int ln = strlen(tx) ;
  int n ;
  if (bpos < 0)
    bpos = lim(ln+bpos-1,0,ln-1) ;
  if (bpos > 0) {
    memcpy(rslt,tx,bpos) ;
    txtpoolidx+= bpos ; }
  if (bpos+len < ln) {
    memcpy(&rslt[bpos],&tx[bpos+len],ln-len-bpos) ;
    txtpoolidx += ln-bpos-len ; }
  txtFixpool() ;
  return rslt ; }
  


// search position in string
int txtPos(tXt src, tXt zk) {
  char * p = strstr(src,zk) ;
  if (p == NULL)
    return txtNOTFOUND ;
  else
    return p-src ; }

char txtErrorBuf[txtMAXLEN] ;


// stitching unlimited number of strings. Last parameter should be NULL
tXt txtConcat(tXt tx1,...) {
  tXt rslt = &txtpool[txtpoolidx] ;
  int txtlim = txtpoolidx+txtMAXLEN ;
  tXt x = tx1 ;
  va_list ap;
  va_start(ap,tx1);
  while (x != NULL) {
    int len = strlen(x) ;
    if (txtpoolidx+len > txtlim)
      len = txtlim-txtpoolidx ;
    memcpy(&txtpool[txtpoolidx],x,len) ;
    txtpoolidx += len ;
    if (txtpoolidx >= txtlim) {
      snprintf(txtErrorBuf,txtMAXLEN,"txtConcat length overflow") ;
      x = NULL ; }
    else
      x = va_arg(ap,tXt) ; }
  va_end(ap);
  txtFixpool() ;
  return rslt ;
}

// check for error
int txtAnyError(void) {
  return txtErrorBuf[0] != 0 ; }

//retrieve and reset error message
tXt txtLastError(void) {
  tXt rslt = txtConcat(txtErrorBuf,NULL) ;
  txtErrorBuf[0] = 0 ; 
  return rslt ; }


//convert to uppercase
tXt txtUpcase(tXt s) {
  tXt rslt = txtConcat(s,NULL) ;
  char * p = rslt ;
  while (*p) {
    if (*p >= 'a' && *p <= 'z')
      *p = *p -'a'+'A' ;
    p++ ; }
  return rslt ; }

// make one char string
tXt txtC(char c) {
  tXt rslt = &txtpool[txtpoolidx] ;
  txtpool[txtpoolidx++] = c ;
  txtFixpool() ;
  return rslt ; }



// create string with standard printf format spec
tXt txtPrintf(const char* format, ...)
{
    va_list argList;
    va_start(argList, format);
    char buf[txtMAXLEN] ;
    int n = vsnprintf(buf,txtMAXLEN-1,format, argList);
    if (n >= txtMAXLEN-1)
      snprintf(txtErrorBuf,txtMAXLEN,"txtPrintf overflow (%d >= %d)",n,txtMAXLEN) ;
    va_end(argList);
    return txtConcat(buf,NULL) ;
}

// allocate memomory in the string pool
tXt txtMalloc(int len) {
  tXt rslt = &txtpool[txtpoolidx] ;
  txtpoolidx += len ;
  txtFixpool() ;
  ZeroMemory(rslt,len) ;
  return rslt ; }

//grab the first part of a string
tXt txtEat(tXt * src,char delim) {
  int p = txtPos(*src,txtC(delim)) ;
  tXt rslt = txtSub(*src,0,p) ;
  *src = &((*src)[mini(p+1,strlen(*src))]) ;
  return rslt ; }

// reverse a string
tXt txtFlip(tXt s) {
  tXt rslt = txtConcat(s,NULL) ;
  char * p = &rslt[strlen(s)-1] ;
  while (*s)
    *p-- = *s++ ; 
  return rslt ; }

// preserve string
tXt fridge(tXt s) {
  int len = strlen(s) ;
  tXt rslt = (char *) malloc(len+2) ;
  strcpy(rslt,s) ;
  rslt[len+1] = 'F' ;
  return rslt ; }


// release fridged string
void clearfridge(tXt s) {
  if (s != NULL && s != &nullchar) {
    int fpod = strlen(s)+ 1 ;
    if (s[fpod] != 'F') {
      char part[10] ;
      memcpy(part,s,9) ;
      part[9] = 0 ;
      snprintf(txtErrorBuf,txtMAXLEN,"false unfridge (%s....)\n",part) ;}
    else {
      s[fpod] = ' ' ;
      free(s) ; } }
}

// un preserve string
tXt unfridge(tXt s) {
  tXt rslt = txtConcat(s,NULL) ;
  clearfridge(s) ;
  return rslt ; }

// put new value in fridged string
void refridge(tXt * p ,tXt nval) {
  clearfridge(*p) ;
  *p = fridge(nval) ; }

// remove spaces around string
tXt txtTrim(tXt tx) {
  int b = 0 ;
  int e = strlen(tx)-1 ;
  int ee = e ;
  while (b <= e && tx[b] == ' ')
    b++ ;
  while (b <= e && tx[e] == ' ')
    e -- ;
  if (b != 0 || e != ee)  
    tx = txtSub(tx,b,e-b+1) ;
  return tx ; }

// replace words in string
tXt txtReplace(tXt src,tXt old,tXt nw) {
  tXt rslt = txtEmpty ;
  int p = txtPos(src,old) ;
  int nrtry = 0 ;
  while (p < txtNOTFOUND && nrtry++ < txtMAXLEN) {
    rslt = txtConcat(rslt,txtSub(src,0,p),nw,NULL) ;
    src = &src[p+strlen(old)] ;
    p = txtPos(src,old) ; }
  rslt = txtConcat(rslt,src,NULL) ;
  return rslt ; }

//read one line from a file
tXt txtFromFile(FILE * fi) {
  char inbuf[txtMAXLEN] ;
  inbuf[0] = 0 ;
  fgets(inbuf,txtMAXLEN,fi) ;
  int li = strlen(inbuf) ; 
  if (li >= txtMAXLEN-1)
    snprintf(txtErrorBuf,txtMAXLEN,"line of %d chars in txtFromFile()",li) ;
  if (li > 0 && inbuf[li-1]=='\n') 
    inbuf[li-1]=0 ; 
  return  txtConcat(inbuf,NULL) ; 
}


#ifndef txtSKIPEXAMP
// some example code
  

// change double slash comments to old style comments
void oldcomment(void) {
  FILE * fi = fopen("milkstrings.c","r") ;
  FILE * fo = fopen("milkocomm.c","w+") ;
  while (!feof(fi)) {
    tXt s = txtFromFile(fi) ;
    int p = txtPos(s,"//") ;
    if ( p == txtNOTFOUND || (p > 0 && s[p-1] == '\"') ) 
      fprintf(fo,"%s\n",s) ;
    else
      fprintf(fo,"%s*/\n",txtReplace(s,"//","/*")) ; }
  fclose(fi) ;
  fclose(fo) ;
}



typedef struct tWord {
  tXt tx ;
  int count ;
  struct tWord *   next ; } tWord;
typedef struct tWord *pWord;

void qwsort(pWord * pp1) {
  pWord p2,p3,p4,tail ;
  int choice ;
  if ((*pp1) && (*pp1)->next) {
    p2 = p3 = *pp1 ;
    while (p2) {
      p2 = p2->next ;
      if (p2) {
        p2 = p2->next ;
        if (p2)
          p3 = p3->next ;
        }}
    p2 = p3->next ;
    p3->next = NULL ;
    qwsort(pp1) ;
    qwsort(&p2) ;
    p3 = *pp1 ;
    tail = NULL ;
    while (p2 != NULL || p3 != NULL) {
      if (p2 != NULL && p3 != NULL)
        choice = strcmp(p2->tx,p3->tx)<0 ;
      else  
        choice = (p2 != NULL) ;
      if (!choice) {
        p4 = p2 ;p2 = p3 ; p3 = p4 ; }
      if (tail) 
        tail->next = p2 ;
      else 
        *pp1 = p2 ;
      tail = p2 ;
      p2 = p2->next ; } }
}
        
    
    

void wordfrequency(void) {
  pWord np,allword ;
  allword = NULL ;
  FILE * fi = fopen("sample.txt","r") ;
  if (fi == NULL)
    return ;
  while (!feof(fi)) {
    tXt lin = txtUpcase(txtTrim(txtFromFile(fi))) ;
    while (lin[0]) {
      tXt wrd = txtTrim(txtEat(&lin,' ')) ;
      np = allword ;
      while (np) {
        if (strcmp(np->tx,wrd) == 0) {
          np->count++ ;
          wrd = "" ; 
          np = NULL ;
          }
        else
          np = np->next ;}
      if (wrd[0]) {
        np = (pWord) malloc(sizeof(tWord)) ;
        np->tx = fridge(wrd) ;
        np->count = 1 ;
        np->next = allword ;
        allword = np ; } } }
  qwsort(&allword) ;      
  while (allword) {
    printf("%5d %s\n",allword->count,unfridge(allword->tx)) ;
    np = allword ;
    allword = allword->next ;
    free (np) ; }
}
      



int main(int argc, char * argv[]) {
  /* oldcomment() ;*/
  /* wordfrequency() ;*/
  if (txtAnyError()) { //check for errors
    printf("%s\n",txtLastError()) ;
    return 1 ; }
  return 0 ;
  tXt s = "123,456,789" ;
  s = txtReplace(s,"123","321") ; // replace 123 by 321
  int num = atoi(txtEat(&s,',')) ; // pick the first number
  printf("num = %d s = %s \n",num,s) ;
  s = txtPrintf("%s,%d",s,num) ; // printf in new string
  printf("num = %d s = %s \n",num,s) ;
  s = txtConcat(s,"<-->",txtFlip(s),NULL) ; // concatenate some strings
  num = txtPos(s,"987") ; // find position of substring
  printf("num = %d s = %s \n",num,s) ;
  s = fridge(s) ;           // preserve string for long time use
  refridge(&s,txtConcat(txtSub(s,4,7),",123",NULL)) ; // update string in fridge ;
  printf("num = %d s = %s \n",num,s) ;
  clearfridge(s) ;          // cleanup heap
  printf("%s\n",txtDelete("0123456789",7,6) ) ;
  if (txtAnyError()) { //check for errors
    printf("%s\n",txtLastError()) ;
    return 1 ; }
  return 0 ;
  }
  
/* output should look like:
num = 321 s = 456,789
num = 321 s = 456,789,321
num = 19 s = 456,789,321<-->123,987,654
num = 19 s = 789,321,123
*/  
  
#endif
