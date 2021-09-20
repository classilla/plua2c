#define MAX_RECORDS 1024

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifdef __LP64__
#include <inttypes.h>
typedef uint8_t UInt8;
typedef uint16_t UInt16;
typedef uint32_t UInt32;
#else
typedef unsigned char UInt8;
typedef unsigned short UInt16;
typedef unsigned long UInt32;
#endif

struct pdbheader_struct {
  UInt8 name[32];
  UInt16 fileAttributes;
  UInt16 version;
  UInt32 creationDate;
  UInt32 modificationDate;
  UInt32 lastBackupDate;
  UInt32 modificationNumber;
  UInt32 appInfoArea;
  UInt32 sortInfoArea;
  UInt8 databaseType[4];
  UInt8 creatorID[4];
  UInt32 uniqueIDSeed;
  UInt32 nextRecordListID;
  UInt16 numberOfRecords;
}

#ifdef __GNUC__
__attribute__ ((packed))
#endif
;

struct recheader_struct {
  UInt32 recordDataOffset;
  UInt8 recordAttributes;
  UInt8 uniqueID[3];
}

#ifdef __GNUC__
__attribute__ ((packed))
#endif
;

struct resheader_struct {
  UInt8 resourceType[4];
  UInt16 resourceID;
  UInt32 recordDataOffset;
}

#ifdef __GNUC__
__attribute__ ((packed))
#endif
;

typedef struct pdbheader_struct pdbheader;
typedef struct recheader_struct recheader;
typedef struct resheader_struct resheader;

typedef struct {
  char *filename;
#ifdef __LP64__
  UInt32 nrecs, resource;
#else
  int nrecs, resource;
#endif
  pdbheader header;
  UInt8 *rec[MAX_RECORDS];
  UInt32 len[MAX_RECORDS];
} pdb_t;

pdb_t *pdb_open(char *filename, char *pdbname, char *creator, char *type, UInt16 attr);
int pdb_close(pdb_t *pdb);
int pdb_addrec(pdb_t *pdb, UInt8 *buf, UInt32 len);
int pdb_addres(pdb_t *pdb, UInt8 *buf, UInt32 len, char *type, UInt16 id);

void pdb_write32(void *addr, UInt32 data);
void pdb_write16(void *addr, UInt16 data);
UInt16 pdb_read16(void *addr);
UInt32 pdb_read32(void *addr);
