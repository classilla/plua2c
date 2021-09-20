#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "pdb.h"

#ifdef __LITTLE_ENDIAN__
#ifndef IS_LITTLE_ENDIAN
#define IS_LITTLE_ENDIAN 1
#endif
#endif

void pdb_write16(void *addr, UInt16 data)
{
  UInt8* to = (UInt8*)addr;
  UInt8* from = (UInt8*)&data;

#ifdef IS_LITTLE_ENDIAN
  to[0] = from[1];
  to[1] = from[0];
#else
  to[0] = from[0];
  to[1] = from[1];
#endif
}

UInt16 pdb_read16(void *addr)
{
  UInt16 data;
  UInt8* to = (UInt8*)&data;
  UInt8* from = (UInt8*)addr;

#ifdef IS_LITTLE_ENDIAN
  to[0] = from[1];
  to[1] = from[0];
#else
  to[0] = from[0];
  to[1] = from[1];
#endif

  return data;
}

void pdb_write32(void *addr, UInt32 data)
{
  UInt8* to = (UInt8*)addr;
  UInt8* from = (UInt8*)&data;

#ifdef IS_LITTLE_ENDIAN
  to[0] = from[3];
  to[1] = from[2];
  to[2] = from[1];
  to[3] = from[0];
#else
  to[0] = from[0];
  to[1] = from[1];
  to[2] = from[2];
  to[3] = from[3];
#endif
}

UInt32 pdb_read32(void *addr)
{
  UInt32 data;
  UInt8* to = (UInt8*)&data;
  UInt8* from = (UInt8*)addr;
  
#ifdef IS_LITTLE_ENDIAN
  to[0] = from[3];
  to[1] = from[2];
  to[2] = from[1];
  to[3] = from[0];
#else
  to[0] = from[0];
  to[1] = from[1];
  to[2] = from[2];
  to[3] = from[3];
#endif

  return data;
}

pdb_t *pdb_open(char *filename, char *pdbname, char *creator, char *type,
                UInt16 attr)
{
  pdb_t *pdb;
  time_t now;

  if ((pdb = calloc(1, sizeof(pdb_t))) == NULL)
    return NULL;

  pdb->filename = strdup(filename);
  pdb->resource = attr & 0x0001;

  now = time(0) + 0x7c25b080l;

  strncpy((char *)pdb->header.name, pdbname, 32);
  pdb_write16(&pdb->header.fileAttributes, attr);
  pdb_write16(&pdb->header.version, 0);
  pdb_write32(&pdb->header.creationDate, now);
  pdb_write32(&pdb->header.modificationDate, now);
  pdb_write32(&pdb->header.lastBackupDate, 0);
  pdb_write32(&pdb->header.modificationNumber, 0);
  pdb_write32(&pdb->header.appInfoArea, 0);
  pdb_write32(&pdb->header.sortInfoArea, 0);
  pdb->header.databaseType[0] = type[0];
  pdb->header.databaseType[1] = type[1];
  pdb->header.databaseType[2] = type[2];
  pdb->header.databaseType[3] = type[3];
  pdb->header.creatorID[0] = creator[0];
  pdb->header.creatorID[1] = creator[1];
  pdb->header.creatorID[2] = creator[2];
  pdb->header.creatorID[3] = creator[3];
  pdb_write32(&pdb->header.uniqueIDSeed, 0);
  pdb_write32(&pdb->header.nextRecordListID, 0);
  pdb_write16(&pdb->header.numberOfRecords, 0);

  return pdb;
}

int pdb_close(pdb_t *pdb)
{
  int i, fd;
  UInt32 offset, size;
  recheader *rech;
  resheader *resh;

  if (!pdb)
    return -1;

  size = pdb->resource ? pdb->nrecs * sizeof(resheader):
                         pdb->nrecs * sizeof(recheader);

  if ((rech = calloc(1, size)) == NULL)
    return -1;

  resh = (resheader *)rech;
  offset = sizeof(pdbheader) + size;

  for (i = 0; i < pdb->nrecs; i++) {
    if (pdb->resource) {
      resh[i].resourceType[0] = pdb->rec[i][0];
      resh[i].resourceType[1] = pdb->rec[i][1];
      resh[i].resourceType[2] = pdb->rec[i][2];
      resh[i].resourceType[3] = pdb->rec[i][3];
      resh[i].resourceID = *((UInt16 *)&pdb->rec[i][4]);
      pdb_write32(&resh[i].recordDataOffset, offset);
    } else {
      pdb_write32(&rech[i].recordDataOffset, offset);
      rech[i].recordAttributes = 0;
    }
    offset += pdb->len[i];
  }

  pdb_write16(&pdb->header.numberOfRecords, pdb->nrecs);

  if ((fd = open(pdb->filename, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, 0644)) == -1) {
    free(pdb);
    return -1;
  }

  write(fd, &pdb->header, sizeof(pdbheader));
  write(fd, rech, size);

  for (i = 0; i < pdb->nrecs; i++) {
    write(fd, pdb->resource ? pdb->rec[i]+6 : pdb->rec[i], pdb->len[i]);
    free(pdb->rec[i]);
  }

  close(fd);

  free(pdb->filename);
  free(pdb);

  return 0;
}

int pdb_addrec(pdb_t *pdb, UInt8 *buf, UInt32 len)
{
  UInt16 index;

  if (!pdb || !buf || !len)
    return -1;

  if (pdb->nrecs == MAX_RECORDS)
   return -1;

  index = pdb->nrecs;

  if ((pdb->rec[index] = calloc(1, len)) == NULL)
    return -1;

  memcpy(pdb->rec[index], buf, len);
  pdb->len[index] = len;
  pdb->nrecs++;

  return index;
}

int pdb_addres(pdb_t *pdb, UInt8 *buf, UInt32 len, char *type, UInt16 id)
{
  UInt16 index;

  if (!pdb || !buf || !len)
    return -1;

  if (pdb->nrecs == MAX_RECORDS)
   return -1;

  index = pdb->nrecs;

  if ((pdb->rec[index] = calloc(1, len+6)) == NULL)
    return -1;

  pdb->rec[index][0] = type[0];
  pdb->rec[index][1] = type[1];
  pdb->rec[index][2] = type[2];
  pdb->rec[index][3] = type[3];
  pdb_write16(pdb->rec[index]+4, id);
  memcpy(pdb->rec[index]+6, buf, len);
  pdb->len[index] = len;
  pdb->nrecs++;

  return index;
}
