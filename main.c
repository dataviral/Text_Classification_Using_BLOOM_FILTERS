/* TOPIC: Bloom Filters
  APPLICATION: Finding the Author of a certain text
  We can train a Number of bloom filters for different authors. If we have a book with unknown author, we can
  find out who the author is!!!
*/
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "ctype.h"
//No of bits
#define bytes_c 1024*512

#define hashsize(n) (1UL << (n))
#define hashmask(n) (hashsize(n) - 1)

#define mix(a,b,c) \
{ \
    a -= b; a -= c; a ^= (c >> 13); \
    b -= c; b -= a; b ^= (a << 8); \
    c -= a; c -= b; c ^= (b >> 13); \
    a -= b; a -= c; a ^= (c >> 12); \
    b -= c; b -= a; b ^= (a << 16); \
    c -= a; c -= b; c ^= (b >> 5); \
    a -= b; a -= c; a ^= (c >> 3); \
    b -= c; b -= a; b ^= (a << 10); \
    c -= a; c -= b; c ^= (b >> 15); \
}
//Structure to represent a bloom
typedef struct bloom{
  char name[30];
  unsigned long long size;
  unsigned short int *bloom_filter;

}BLOOM;
//structure which contains all the bloom filters
typedef struct bloom_container{
  int n;
  BLOOM *bloom_array[200];
}BLOOM_CONTAINER;

unsigned long long jenkins_hash(unsigned char *, unsigned );
unsigned long long fnv_hash(void * , int );
unsigned long long oat_hash(void * , int );
unsigned long long elf_hash(void *, int );
unsigned long long djb2(unsigned char *str);


BLOOM *create_new_bloom(int);
void insert_into_bloom(BLOOM *, char *);
int contained_in_bloom(BLOOM *, char *);
void save(BLOOM *);
BLOOM *read_from_file( char *);
void flush();
void insert_from_file(BLOOM *, char *);
BLOOM *check_master();

//Hashing function named Jenkins hash
unsigned long long jenkins_hash(unsigned char *k, unsigned length)
{
    unsigned long long a, b;
    unsigned long long c = 37;
    unsigned len = length;

    a = b = 0x9e3779b9;

    while (len >= 12)
    {
        a += (k[0] + ((unsigned long long)k[1] << 8) + ((unsigned long long)k[2] << 16) + ((unsigned long long)k[3] << 24));
        b += (k[4] + ((unsigned long long)k[5] << 8) + ((unsigned long long)k[6] << 16) + ((unsigned long long)k[7] << 24));
        c += (k[8] + ((unsigned long long)k[9] << 8) + ((unsigned long long)k[10] << 16) + ((unsigned long long)k[11] << 24));

        mix(a, b, c);

        k += 12;
        len -= 12;
    }

    c += length;
    switch (len)
    {
    case 11: c += ((unsigned long long)k[10] << 24);
    case 10: c += ((unsigned long long)k[9] << 16);
    case 9: c += ((unsigned long long)k[8] << 8);
        /* First byte of c reserved for length */
    case 8: b += ((unsigned long long)k[7] << 24);
    case 7: b += ((unsigned long long)k[6] << 16);
    case 6: b += ((unsigned long long)k[5] << 8);
    case 5: b += k[4];
    case 4: a += ((unsigned long long)k[3] << 24);
    case 3: a += ((unsigned long long)k[2] << 16);
    case 2: a += ((unsigned long long)k[1] << 8);
    case 1: a += k[0];
    }

    mix(a, b, c);


    return c;
}
//Hashing function named 
unsigned long long fnv_hash(void *key, int len)
{
    unsigned char *p = key;
    unsigned long long h = 2166136261;
    int i;

    for (i = 0; i < len; i++)
    {
        h = (h * 16777619) ^ p[i];
    }

    return h;
}

unsigned long long oat_hash(void *key, int len)
{
    unsigned char *p = key;
    unsigned long long h = 0;
    int i;

    for (i = 0; i < len; i++)
    {
        h += p[i];
        h += (h << 10);
        h ^= (h >> 6);
    }

    h += (h << 3);
    h ^= (h >> 11);
    h += (h << 15);

    return h;
}

unsigned long long elf_hash(void *key, int len)
{
    unsigned char *p = key;
    unsigned long long  h = 0, g;
    int i;

    for (i = 0; i < len; i++)
    {
        h = (h << 4) + p[i];
        g = h & 0xf0000000L;

        if (g != 0)
        {
            h ^= g >> 24;
        }

        h &= ~g;
    }

    return h;
}

unsigned long long djb2(unsigned char *str)
{
    unsigned long long hash = 5381;
    int c;
    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}


BLOOM *check_master(){
  BLOOM *master = read_from_file("bloom_filters/master_750");
  return master;
}



BLOOM_CONTAINER *create_bloom_container(){
  BLOOM_CONTAINER *temp = (BLOOM_CONTAINER *)malloc(sizeof(BLOOM_CONTAINER));
  temp->n = -1;
  return temp;
}

BLOOM *create_new_bloom(int bloom_size){
  BLOOM *temp = (BLOOM *)malloc(sizeof(BLOOM));
  //temp->name = (char *)malloc(25);
  temp->bloom_filter = (short int *)malloc( sizeof(int) * bloom_size * bytes_c);
  temp->size = bloom_size * bytes_c  ;

  fprintf(stdout,"\nEnter The \'Name\' of the Bloom Filter : ");
  fscanf(stdin, "%s", temp->name);
  return temp;
}

BLOOM *select_bloom(BLOOM_CONTAINER *b){
  fprintf(stdout,"\tSelect one of the following (-1 to cancel)\n");
  int i;
  for(i=0;i<=b->n;i++){
    fprintf(stdout,"%d : %s\n",i+1, (b->bloom_array[i])->name);
  }
  fprintf(stdout,"\nEnter your Choice :");
  fscanf(stdin,"%d", &i);
  i--;
  if(i>=0 && i<= b->n)
    return b->bloom_array[i];
  return 0;
}

void save(BLOOM *b){
  fprintf(stdout,"Saving to file.....\n");
  char path[100] = "bloom_filters/";
  strcat(path, b->name);
  printf("%s\n",path );
  FILE *file = fopen(path, "w+");
  fprintf(file, "%s\n", b->name);
  fprintf(file, "%Lu\n", b->size);
  int i;
  for(i=0;i<b->size;i++){
  fprintf(file, "%hu \n", b->bloom_filter[i]);
  }
  fclose(file);
  getchar();
}

BLOOM *read_from_file(char *path){


  FILE *f = NULL;
  BLOOM *b = NULL;
  f= fopen(path,"r");
  if(f){
    b = (BLOOM *)malloc(sizeof(BLOOM));
    int i=0;
    fscanf(f, "%s\n", b->name);
    fscanf(f, "%Lu\n", &(b->size));
    b->bloom_filter = (unsigned short int *)malloc((sizeof(int))*(b->size));
    for(i=0;i<b->size;i++){
      fscanf(f,"%hu\n",&(b->bloom_filter[i]));
      }

    fclose(f);
    return b;
  }
  perror("Error");
  }

void insert_into_bloom(BLOOM *bloom, char *ele){
    int len = strlen(ele);
    bloom->bloom_filter[jenkins_hash(ele, len)%bloom->size] = 1 ;
    bloom->bloom_filter[fnv_hash(ele, len)%bloom->size] = 1 ;
    bloom->bloom_filter[oat_hash(ele, len)%bloom->size] = 1 ;
    bloom->bloom_filter[elf_hash(ele, len)%bloom->size] = 1 ;
    bloom->bloom_filter[djb2(ele)%bloom->size] = 1 ;

}

int contained_in_bloom(BLOOM *bloom, char *ele){
  int len = strlen(ele);
  if( bloom->bloom_filter[jenkins_hash(ele, len)%bloom->size] && bloom->bloom_filter[fnv_hash(ele, len)%bloom->size] && bloom->bloom_filter[oat_hash(ele, len)%bloom->size] &&   bloom->bloom_filter[elf_hash(ele, len)%bloom->size] && bloom->bloom_filter[djb2(ele)%bloom->size] ){
    return 1;
  }
}

void insert_from_file(BLOOM *bloom, char *file){
  char c;
  FILE *f = fopen(file, "r");
  if(f){
  char word[20];
  int i = 0,x=0;
  unsigned long long now = 0;
  BLOOM *master = check_master();
  while( (c= getc(f)) != EOF ){
    if(c >= 'a' && c <='z' || c <='Z' && c>= 'A'){
      word[i] = tolower(c)  ;
      i++;
      x=0;
    }
   else if(contained_in_bloom(master, word)){
     memset(word,0,sizeof(word));
     i=0;
     x=1;
    }
      else {
          if(x==0 && strlen(word)>3){
            printf("%s\n", word );
            insert_into_bloom(bloom,word);
            now++;
            }
            memset(word,0,sizeof(word));
            i=0;
            x=1;
          }

      }
  printf("No of words inserted :%Ld\n", now);
  fclose(f);
  }
  else
    printf("No Such file exists\n");
}

int compare_blooms(BLOOM *bloom1, BLOOM *bloom2, char *file){

  FILE *f = fopen(file, "r");
  if(f){
    char c;
    unsigned long long b1=0,b2=0;
    char word[100];
    int i = 0, x=0;
  BLOOM *master = check_master();
    while( (c= getc(f)) != EOF ){
      if(c >= 'a' && c <='z' || c <='Z' && c>= 'A'){
        word[i] = tolower(c)  ;
        i++;
        x=0;
      }
        else if(x==0){
        if(contained_in_bloom(master, word)){

        }
          else{
                  if(strlen(word)>3){
                  if(contained_in_bloom(bloom1,word)){
                    b1++;
                  }
                  if(contained_in_bloom(bloom2,word)){
                    b2++;
                  }

                }

              }
              x=1;
              i=0;
              memset(word,0,sizeof(word));
      }
    }
    printf("%Ld %Ld\n", b1, b2);
    fclose(f);
    return b1>b2 ? 1:-1;
  }
  else
    printf("No such file exists\n");

}



void flush(){
  int ch;
  while ((ch=getchar()) != EOF && ch != '\n');
}



int main(void){

  int ch1;
  BLOOM_CONTAINER *b = create_bloom_container();
  BLOOM *selected_bloom = NULL;
  while(1){

    system("clear");
    fprintf(stdout,"\n1.Create a new Bloom Filter\n2.Select a Bloom filter\n3.Read Bloom Filter From File\n4.Compare two bloom\n5.Exit\nEnter Your Choice : ");
    fscanf(stdin,"%d",&ch1);
    switch (ch1) {
      case 1:{
              fprintf(stdout,"\nEnter the size of the new Bloom Filter (-1 to cancel):");
              int ele;
              fscanf(stdin,"%d",&ele);
              if(ele > 0){
              b->n++;
              b->bloom_array[b->n] = create_new_bloom(ele);
              flush();
            }
            }
            break;
    case 2:{
            selected_bloom = select_bloom(b);
            if(selected_bloom != NULL){
                  int ch2 = 0;
                  int c;
                  while(ch2 != 5){
                  ch2=0;
                  fprintf(stdout,"\n1.insert\n2.check\n3.save\n4.Insert From file\n5.go back\nEnter Your Choice :");
                  fscanf(stdin,"%d",&ch2);
                  switch(ch2){
                    case 1:
                          {
                            char ele[100];
                            fprintf(stdout,"\nEnter the element to be inserted :");
                            fscanf(stdin, "%s", ele);
                            flush();
                            insert_into_bloom(selected_bloom, ele);
                          }
                          break;

                    case 2:
                          {
                            char ele[100];
                            fprintf(stdout,"\nEnter the element to be checked :");
                            fscanf(stdin, "%s", ele);
                            flush();

                          if(contained_in_bloom(selected_bloom, ele))
                            fprintf(stdout,"It Might be Present\n");
                          else
                            fprintf(stdout,"Nein! It's Not There\n");
                          }
                          break;

                    case 3:

                          save(selected_bloom);
                          break;

                    case 4:
                          {
                              char file[25];
                              fprintf(stdout, "\n Enter the file name :");
                              fscanf(stdin, "%s", file);
                              char path[100] = "load_from/";
                              strcat(path, file);
                              flush();
                              insert_from_file(selected_bloom, path);
                          }

                          break;


                    case 5:
                          break;

                    default :
                          fprintf(stdout,"\nInvalid Choice !\n");
                    }
                    getchar();
                  }
              }
              else
              fprintf(stdout,"\nNo Bloom Filter Selected\n");
              flush();
            }
            break;
      case 3:{
          fprintf(stdout,"Enter the filename : ");
          char file[50];
          fscanf(stdin, "%s", file);
          flush();
          char path[100] = "bloom_filters/";
          strcat(path, file);
            BLOOM *x = read_from_file( path);
            if(x != NULL){
              b->n++;
              b->bloom_array[b->n] = x;
            }
        }
          getchar();
          break;

      case 4:{
          printf("Select bloom 1 and 2\n");
          BLOOM *b1 = select_bloom(b), *b2 =select_bloom(b);
          char path[100] = "load_from/",file[20];
          printf("Enter the file name to be compared from :");
          fscanf(stdin,"%s",file);
          flush();
          strcat(path,file);
          int x = compare_blooms(b1,b2,path);
          if(x==1)
            printf("The text is written by %s \n", b1->name);
          else
            printf("The text is written by %s \n", b2->name);
          getchar();
      }
        break;
      case 5:
          exit(0);
        }
      }

}
