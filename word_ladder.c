//
// AED, November 2022 (Tomás Oliveira e Silva)
//
// Second practical assignement (speed run)
//
// Place your student numbers and names here
//   N.Mec. 97689  Name: José Luca Baptista Pereira
//
// Do as much as you can
//   1) MANDATORY: complete the hash table code
//      *) hash_table_create  FEITO
//      *) hash_table_grow    FEITO
//      *) hash_table_free    FEITO
//      *) find_word          FEITO
//      +) add code to get some statistical data about the hash table
//   2) HIGHLY RECOMMENDED: build the graph (including union-find data) -- use the similar_words function...
//      *) find_representative    FEITO
//      *) add_edge               FEITO
//   3) RECOMMENDED: implement breadth-first search in the graph
//      *) breadh_first_search
//   4) RECOMMENDED: list all words belonginh to a connected component
//      *) breadh_first_search
//      *) list_connected_component
//   5) RECOMMENDED: find the shortest path between to words
//      *) breadh_first_search
//      *) path_finder
//      *) test the smallest path from bem to mal
//         [ 0] bem
//         [ 1] tem
//         [ 2] teu
//         [ 3] meu
//         [ 4] mau
//         [ 5] mal
//      *) find other interesting word ladders
//   6) OPTIONAL: compute the diameter of a connected component and list the longest word chain
//      *) breadh_first_search
//      *) connected_component_diameter
//   7) OPTIONAL: print some statistics about the graph
//      *) graph_info
//   8) OPTIONAL: test for memory leaks
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//
// static configuration
//

#define _max_word_size_  32


//
// data structures (SUGGESTION --- you may do it in a different way)
//

typedef struct adjacency_node_s  adjacency_node_t;
typedef struct hash_table_node_s hash_table_node_t;
typedef struct hash_table_s      hash_table_t;

struct adjacency_node_s
{
  adjacency_node_t *next;            // link to the next adjacency list node
  hash_table_node_t *vertex;         // the other vertex
};

struct hash_table_node_s
{
  // the hash table data
  char word[_max_word_size_];        // the word
  hash_table_node_t *next;           // next hash table linked list node
  // the vertex data
  adjacency_node_t *head;            // head of the linked list of adjancency edges
  int visited;                       // visited status (while not in use, keep it at 0)
  hash_table_node_t *previous;       // breadth-first search parent
  // the union find data
  hash_table_node_t *representative; // the representative of the connected component this vertex belongs to
  int number_of_vertices;            // number of vertices of the conected component (only correct for the representative of each connected component)
  int number_of_edges;               // number of edges of the conected component (only correct for the representative of each connected component)
};

struct hash_table_s
{
  unsigned int hash_table_size;      // the size of the hash table array
  unsigned int number_of_entries;    // the number of entries in the hash table
  unsigned int number_of_edges;      // number of edges (for information purposes only)
  hash_table_node_t **heads;         // the heads of the linked lists
};


//
// allocation and deallocation of linked list nodes (done)
//

static adjacency_node_t *allocate_adjacency_node(void)
{
  adjacency_node_t *node;

  node = (adjacency_node_t *)malloc(sizeof(adjacency_node_t));
  if(node == NULL)
  {
    fprintf(stderr,"allocate_adjacency_node: out of memory\n");
    exit(1);
  }
  return node;
}

static void free_adjacency_node(adjacency_node_t *node)
{
  free(node);
}

static hash_table_node_t *allocate_hash_table_node(void)
{
  hash_table_node_t *node;

  node = (hash_table_node_t *)malloc(sizeof(hash_table_node_t));
  if(node == NULL)
  {
    fprintf(stderr,"allocate_hash_table_node: out of memory\n");
    exit(1);
  }
  return node;
}

static void free_hash_table_node(hash_table_node_t *node)
{
  free(node);
}


//
// hash table stuff (mostly to be done)
//

unsigned int crc32(const char *str)
{
  static unsigned int table[256];
  unsigned int crc;

  if(table[1] == 0u) // do we need to initialize the table[] array?
  {
    unsigned int i,j;

    for(i = 0u;i < 256u;i++)
      for(table[i] = i,j = 0u;j < 8u;j++)
        if(table[i] & 1u)
          table[i] = (table[i] >> 1) ^ 0xAED00022u; // "magic" constant
        else
          table[i] >>= 1;
  }
  crc = 0xAED02022u; // initial value (chosen arbitrarily)
  while(*str != '\0')
    crc = (crc >> 8) ^ table[crc & 0xFFu] ^ ((unsigned int)*str++ << 24);
  return crc;
}

static hash_table_t *hash_table_create(void)
{
  hash_table_t *hash_table;
  unsigned int i;

  hash_table = (hash_table_t *)malloc(sizeof(hash_table_t));
  if(hash_table == NULL)
  {
    fprintf(stderr,"Out of memory");
    exit(1);
  }
   hash_table->number_of_entries = 0;
   hash_table->hash_table_size = 512;
   hash_table->number_of_edges = 0;
   hash_table->heads = calloc(hash_table->hash_table_size , sizeof(hash_table_node_t));
   if(hash_table->heads == NULL) {
      fprintf(stderr,"Out of memory");
      exit(1);  
   }
   return hash_table;
}


static void hash_table_grow(hash_table_t *hash_table)
{
  hash_table_node_t **prev_heads , **heads;
  hash_table_node_t *node,*next_node;
  unsigned int i;

  //Store the previous heads
  prev_heads = hash_table->heads;
  //Get new capacity (twice the current)
  unsigned int new_capacity = hash_table->hash_table_size * 2;
  
  if (new_capacity < hash_table->hash_table_size) { // overflow (capacity would be too big)
      fprintf(stderr , "Overflow");
      exit(1);  
  }
  // Allocate new extra space
  heads = (hash_table_node_t **)malloc(hash_table->hash_table_size * sizeof(hash_table_node_t *));
  // Double the size of the table
  unsigned int prev_size = hash_table->hash_table_size;
  hash_table->hash_table_size = new_capacity;
 

  //we run the hash fucntion for the previousv values we stored above and stored them in the new resized table
  for(i = 0;i < prev_size;i++){
    node = prev_heads[i];
    while(node != NULL){
      
      next_node = node->next;
      
      size_t index = crc32(node->word) % hash_table->hash_table_size;
      node->next = heads[index];
      heads[index] = node;

      node = next_node;
    }
  }
  free(prev_heads); //free the stored values (no longer needed)
  hash_table->heads = heads;
  
}

static void hash_table_free(hash_table_t *hash_table)
{
  hash_table_node_t *node,*next_node;
  unsigned int i;

  for (i = 0; i < hash_table->hash_table_size; i++) //Loop through all the heads of the linked lists
  {
    hash_table_node_t *node = hash_table->heads[i];  //Get the head for current index
    while (node != NULL)    //Check if current node is NULL
    {
      hash_table_node_t *next = node->next; //Get the next node in the linked list
      free_hash_table_node(node);   //Free the node
      node = next;    //Used the next node as current node
    }
  }
  free(hash_table->heads); //Free the lsit of heads
  free(hash_table); //Free the actual hash table
}



static hash_table_node_t *find_word(hash_table_t *hash_table,const char *word,int insert_if_not_found)
{
  hash_table_node_t *node;
  unsigned int i;

  i = crc32(word) % hash_table->hash_table_size;   //Hash the word to figure out the index in the hash table
  
  node = hash_table->heads[i];                    //Get the head of the linked list to where the hashed word pints to
  
  //Loop through all the nodes in the linked list, if the word is in one of them reutrn that node
  while (node != NULL) {
    if(strcmp(node->word , word) == 0) {          
      return node; 
    }
  }

  //If the word is not in the table and insert_if_not_found is 1 then insert the word in the table
  if(insert_if_not_found && (strlen(word) <= _max_word_size_)) {
    //create the node
    node = allocate_hash_table_node();
    strncpy(node->word,word,_max_word_size_);
    node->representative = node;
    node->next = hash_table->heads[i];
    node->previous = NULL;
    node->head = NULL;
    node->number_of_edges = 0;
    node->number_of_vertices = 1;
    node-> visited = 0;
    hash_table->heads[i] = node;
    hash_table->number_of_entries++;
    if(hash_table->number_of_entries > hash_table->hash_table_size)
      hash_table_grow(hash_table);
    return node;
  }
  return NULL;
}


//
// add edges to the word ladder graph (mostly do be done)
//

static hash_table_node_t *find_representative(hash_table_node_t *node)
{
  hash_table_node_t *representative,*next_node;

   for(representative = node; representative != representative->representative; representative = representative->representative)
    ;
  
  return representative;
}
  
  


static void add_edge(hash_table_t *hash_table,hash_table_node_t *from,const char *word)
{
  hash_table_node_t *to,*from_representative,*to_representative;
  adjacency_node_t *link,*linkop;

  to = find_word(hash_table,word,0);
  from_representative = find_representative(from); //get the from representative

  if (to != NULL) { //If the to exists in the table
    to_representative = find_representative(to); //get the to representative
  } else {
    return;
  }

  if(to == from) { //check if to and from are the same, if they are no edge should be added
    return;
  }

  if(to_representative == from_representative) {
    to_representative->number_of_edges++;
      
  } else if (from_representative->number_of_vertices < to_representative->number_of_vertices) {
    to_representative->number_of_edges++;
    to_representative->number_of_vertices += from_representative->number_of_vertices;
    to_representative->number_of_edges += from_representative->number_of_edges;
    from_representative->representative = to_representative;
  } else {
    from_representative->number_of_edges++;
    from_representative->number_of_vertices += to_representative->number_of_vertices;
    from_representative->number_of_edges += to_representative->number_of_edges;
    to_representative->representative = from_representative;
  }


  link = allocate_adjacency_node();
  linkop = allocate_adjacency_node();


  linkop->vertex = from;
  linkop->next = to->head;
  to->head = linkop;

  link->vertex = to;
  link->next = from->head;
  from->head = link;

  hash_table->number_of_edges++;
  return;


}


//
// generates a list of similar words and calls the function add_edge for each one (done)
//
// man utf8 for details on the uft8 encoding
//

static void break_utf8_string(const char *word,int *individual_characters)
{
  int byte0,byte1;

  while(*word != '\0')
  {
    byte0 = (int)(*(word++)) & 0xFF;
    if(byte0 < 0x80)
      *(individual_characters++) = byte0; // plain ASCII character
    else
    {
      byte1 = (int)(*(word++)) & 0xFF;
      if((byte0 & 0b11100000) != 0b11000000 || (byte1 & 0b11000000) != 0b10000000)
      {
        fprintf(stderr,"break_utf8_string: unexpected UFT-8 character\n");
        exit(1);
      }
      *(individual_characters++) = ((byte0 & 0b00011111) << 6) | (byte1 & 0b00111111); // utf8 -> unicode
    }
  }
  *individual_characters = 0; // mark the end!
}

static void make_utf8_string(const int *individual_characters,char word[_max_word_size_])
{
  int code;

  while(*individual_characters != 0)
  {
    code = *(individual_characters++);
    if(code < 0x80)
      *(word++) = (char)code;
    else if(code < (1 << 11))
    { // unicode -> utf8
      *(word++) = 0b11000000 | (code >> 6);
      *(word++) = 0b10000000 | (code & 0b00111111);
    }
    else
    {
      fprintf(stderr,"make_utf8_string: unexpected UFT-8 character\n");
      exit(1);
    }
  }
  *word = '\0';  // mark the end
}

static void similar_words(hash_table_t *hash_table,hash_table_node_t *from)
{
  static const int valid_characters[] =
  { // unicode!
    0x2D,                                                                       // -
    0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,           // A B C D E F G H I J K L M
    0x4E,0x4F,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,           // N O P Q R S T U V W X Y Z
    0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,           // a b c d e f g h i j k l m
    0x6E,0x6F,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,           // n o p q r s t u v w x y z
    0xC1,0xC2,0xC9,0xCD,0xD3,0xDA,                                              // Á Â É Í Ó Ú
    0xE0,0xE1,0xE2,0xE3,0xE7,0xE8,0xE9,0xEA,0xED,0xEE,0xF3,0xF4,0xF5,0xFA,0xFC, // à á â ã ç è é ê í î ó ô õ ú ü
    0
  };
  int i,j,k,individual_characters[_max_word_size_];
  char new_word[2 * _max_word_size_];

  break_utf8_string(from->word,individual_characters);
  for(i = 0;individual_characters[i] != 0;i++)
  {
    k = individual_characters[i];
    for(j = 0;valid_characters[j] != 0;j++)
    {
      individual_characters[i] = valid_characters[j];
      make_utf8_string(individual_characters,new_word);
      // avoid duplicate cases
      if(strcmp(new_word,from->word) > 0)
        add_edge(hash_table,from,new_word);
    }
    individual_characters[i] = k;
  }
}


//
// breadth-first search (to be done)
//
// returns the number of vertices visited; if the last one is goal, following the previous links gives the shortest path between goal and origin
//

static int breadh_first_search(int maximum_number_of_vertices,hash_table_node_t **list_of_vertices,hash_table_node_t *origin,hash_table_node_t *goal)
{
  
}


//
// list all vertices belonging to a connected component (complete this)
//

static void list_connected_component(hash_table_t *hash_table,const char *word)
{
  //
  // complete this
  //
}


//
// compute the diameter of a connected component (optional)
//

static int largest_diameter;
static hash_table_node_t **largest_diameter_example;

static int connected_component_diameter(hash_table_node_t *node)
{
  int diameter;

  //
  // complete this
  //
  return diameter;
}


//
// find the shortest path from a given word to another given word (to be done)
//

static void path_finder(hash_table_t *hash_table,const char *from_word,const char *to_word)
{

}


//
// some graph information (optional)
//

static void graph_info(hash_table_t *hash_table)
{
  //
  // complete this
  //
}


//
// main program
//

int main(int argc,char **argv)
{
  char word[100],from[100],to[100];
  hash_table_t *hash_table;
  hash_table_node_t *node;
  unsigned int i;
  int command;
  FILE *fp;

  // initialize hash table
  hash_table = hash_table_create();
  // read words
  fp = fopen((argc < 2) ? "wordlist-big-latest.txt" : argv[1],"rb");
  if(fp == NULL)
  {
    fprintf(stderr,"main: unable to open the words file\n");
    exit(1);
  }
  while(fscanf(fp,"%99s",word) == 1)
    (void)find_word(hash_table,word,1);
  fclose(fp);
  hash_table_print(hash_table);
/*
  // find all similar words
  for(i = 0u;i < hash_table->hash_table_size;i++)
    for(node = hash_table->heads[i];node != NULL;node = node->next)
      similar_words(hash_table,node);
  graph_info(hash_table);
  // ask what to do
  for(;;)
  {
    fprintf(stderr,"Your wish is my command:\n");
    fprintf(stderr,"  1 WORD       (list the connected component WORD belongs to)\n");
    fprintf(stderr,"  2 FROM TO    (list the shortest path from FROM to TO)\n");
    fprintf(stderr,"  3            (terminate)\n");
    fprintf(stderr,"> ");
    if(scanf("%99s",word) != 1)
      break;
    command = atoi(word);
    if(command == 1)
    {
      if(scanf("%99s",word) != 1)
        break;
      list_connected_component(hash_table,word);
    }
    else if(command == 2)
    {
      if(scanf("%99s",from) != 1)
        break;
      if(scanf("%99s",to) != 1)
        break;
      path_finder(hash_table,from,to);
    }
    else if(command == 3)
      break;
  }
  */
  // clean up
  hash_table_free(hash_table);
  return 0;
}
