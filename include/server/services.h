typedef struct doc Document;

int addDoc (char* title, char* author, short year, char* fileName, char* pathDocs);

char* consultDoc (int id);

int deleteDoc (int id);