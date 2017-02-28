#include <iostream>
#include "getopt.h"
#include <math.h>
#include "EicDetConfig.h"

#define no_argument 0
#define required_argument 1 
#define optional_argument 2

void print_usage();
void print_libs();
void print_cflags();
void print_ldflags();
void print_inc();
void print_version();
void print_grid();
void print_prefix();

int main(int argc, char * argv[]) {

   if( argc < 2 ){
      print_usage();
      std::cout << std::endl;
      return 0;
   }
   const struct option longopts[] =
   {
      {"version",   no_argument,  0, 'v'},
      {"help",      no_argument,  0, 'h'},
      {"libs",      no_argument,  0, 'l'},
      {"cflags",    no_argument,  0, 'c'},
      {"ldflags",   no_argument,  0, 'd'},
      {"inc",       no_argument,  0, 'i'},
      {"grid",      no_argument,  0, 'g'},
      {"prefix",    no_argument,  0, 'p'},
      {0,0,0,0}
   };

   int index = 0;
   int iarg  = 0;

   //turn off getopt error message
   opterr=1; 

   while(iarg != -1)
   {
      iarg = getopt_long(argc, argv, "vhlcpd", longopts, &index);

      switch (iarg)
      {
         case 'h':
            print_usage();
            break;

         case 'v':
            print_version();
            break;

         case 'l':
            print_libs();
            break;

         case 'c':
            print_cflags();
            break;

         case 'i':
            print_inc();
            break;

         case 'd':
            print_ldflags();
            break;

         case 'p':
            print_prefix();
            break;

         case 'g':
            print_grid();
            break;


      }
   }

   std::cout << std::endl;

   return 0; 
}

void print_version(){
   std::cout << "EicDet Version " << EicDet_MAJOR_VERSION << "." << EicDet_MINOR_VERSION << "." << EicDet_PATCH_VERSION << " ";
}

void print_usage(){
   std::cout << "EicDet-config --libs --cflags --ldflags --inc --grid" << " ";
}

void print_libs(){
   std::cout << EicDet_CXX_LIBS << " ";
}

void print_inc(){
   std::cout << EicDet_CXX_INC_DIR << " ";
}

void print_cflags(){
   std::cout << EicDet_CXX_CFLAGS << " ";
}

void print_ldflags(){
   std::cout << EicDet_CXX_LDFLAGS << " ";
}

void print_grid(){
   std::cout << EicDet_DATA_DIR << " ";
}

void print_prefix(){
   std::cout << EicDet_PREFIX << " ";
}

