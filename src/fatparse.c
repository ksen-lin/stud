#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
//https://stackoverflow.com/questions/526430/c-programming-how-to-program-for-unicode
#include <uchar.h>
#include <wchar.h>

#define ATTR_RO        0x01
#define ATTR_HIDDEN    0x02
#define ATTR_SYSTEM    0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE   0x20
#define ATTR_RESERVED  0x40

#pragma pack(push, 1)
struct biospb {
    short sector_size;
    char  sectincluster;
    short ressects;    // 0x0e
    char  fat_count;
    short root_fd_num;       // 0x11
    short sectors;
    char  media;
    short fat_sectors;  // 0x16
    short track_sectors; // 0x18
    short heads;
    int   hiddensects;
    int   largesects;
    char  phdisc_num;
    char  curr_head; // not used
    char  sig;
    int   serial; // volume serial
    char  label[11];
    char  systemid[8];
};

// fat32 only
struct fsinfo {

};

struct fat12bs {
    char  jmp[3];
    char  id[8];
    struct biospb bpb;
    struct biospb ebpb;
    int info;
};

struct long_filename {
    char LDIR_ord;
    char16_t name1[5];
    char LDIR_attr;
    char LDIR_type;
    char LDIR_chksum;

    char16_t name2[6];
    short LDIR_first_clus_lo;
    char16_t name3[2];
};

struct file_record {
    char name[11];
    char attr;
    char nt;
    char crt_time10th;
    short crt_time;
    short crt_date;
    short last_acc;
    short fst_clus_hi;
    short wr_time;
    short wr_date;
    short fst_clus_lo;
    int   fsize;
};

// fat16
// 0000: free,
// 0002-ffef: cluster in use; the value given is the number of the next cluster in the file,
// fff0-fff6: reserved,
// fff7: bad cluster,
// fff8-ffff: cluster in use, the last one in this file
//  Now the maximum possible size of a FAT16 volume is 2 GB (65526 clusters of at most 64 sectors each).

#pragma pack(pop)

struct options {
    char verbose;
};

/* * * * * * * * * * * * * *
 *      G L O B A L S      *
 * * * * * * * * * * * * * */
struct options opts = {0};
struct fat12bs bootsect;
short *fat = (void *)0;
int    fat_n = 0;


void print_lfn(struct long_filename *addr)
{
    int i;
    for (i = 0; i < 5; i += 1) {
        putchar(addr->name1[i]);
    }
//         putchar(((struct long_filename *)(&root_dir[i]))->name1[i]);
    for (i = 0; i < 6; i += 1) {
        putchar(addr->name2[i]);
    }
//         putchar(((struct long_filename *)(&root_dir[i]))->name2[i]);
    for (i = 0; i < 2; i += 1) {
        putchar(addr->name3[i]);
    }
//         putchar(((struct long_filename *)(&root_dir[i]))->name3[i]);
//                 putchar(root_dir[i].lfn.name3[i]);
    return;
}



// int inline fat_next_rec(rec)
// {
//     return fat[];
// }



short last_clus(short clus)
{
    if (!clus)
        return 0;

    while ((short)0xffff != fat[clus]) {
        clus = fat[clus];
    }
    printf(" [+] found last cluster: %#hx\n", clus);
    return clus;
}


void show_clus_chain(short clus)
{
    if (!clus)
        return;

    printf("   clusters:\n");
    while (clus != (short)0xffff ) {
        printf("%#x -> ", clus);
        clus = fat[clus];
    }
    printf("%#6hx [EOF]\n", clus);
}


void print_file(struct file_record *frec,
                short cluster_size,
                short sec_per_clus,
                int fst_data_sect)
{
    int clus_no   = ((frec->fst_clus_hi << 16) | frec->fst_clus_lo),
        base_sect = sec_per_clus * (clus_no-2) + fst_data_sect,
        tmp;
    printf("  |  name:  \"%.11s\"\n", frec->name);
    printf("  |  size: %i bytes\n",  frec->fsize);
    printf("  |  start:  offset %#8x  sector %#x (clus %#x)\n",
           base_sect*cluster_size/sec_per_clus, base_sect, clus_no);

    if (opts.verbose)
        show_clus_chain(clus_no);

    if (frec->fsize % cluster_size) {
        tmp = last_clus(clus_no);
        printf(" ~~~ %u bytes available in the trailing block ~~~\n"
               "   you can find it in cluster %#x (offset %#x)\n",
                cluster_size - frec->fsize % cluster_size, tmp,
                cluster_size/sec_per_clus*((tmp-2)*sec_per_clus + fst_data_sect));
    }
}

void parse_dir(struct file_record *dir)
{

    return;
}

int main(int argc, char *argv[])
{
    if (argc > 1)
        opts.verbose = 1;

    errno = 0;
    int i, tmp;
    int first_data_sector, root_dir_clus, root_dir_sectors;

    FILE *f = fopen("adams.dd", "r");
    perror("fopen");

    /* reading FAT16 boot sector */
    fread(&bootsect, 1, sizeof(struct fat12bs), f);

    printf(" ~~~ G E N E R A L   F S   I N F O ~~~\n");
    printf("                 id: \"%.4s\"\n", bootsect.id);
    printf("        sector_size: %i\n", bootsect.bpb.sector_size);
    printf("sectors per cluster: %i\n", bootsect.bpb.sectincluster);
    printf("        fat_sectors: %i\n", (int)bootsect.bpb.fat_sectors);
    printf("               FATs: %i\n", (int)bootsect.bpb.fat_count);
    printf("        root_fd_num: %i\n", bootsect.bpb.root_fd_num);
    printf("      volume serial: %#x\n", bootsect.bpb.serial);
    printf("       volume label: \"%.11s\"\n", bootsect.bpb.label);
    printf("          system_id: \"%.8s\"\n", bootsect.bpb.systemid);
    printf("             fsinfo: %#x\n", bootsect.info);
    printf("   root_dir_sectors: %i\n", root_dir_sectors =
           32*bootsect.bpb.root_fd_num/bootsect.bpb.sector_size);

    /* reading 1st FAT */
    fseek(f, 512, 0);
    tmp = bootsect.bpb.fat_sectors * bootsect.bpb.sector_size;
    fat_n = tmp/sizeof(short);
    fat = malloc(tmp);
    fread(fat, 1, tmp, f);

    /* reading root directory*/
    root_dir_clus = 1 + 2*bootsect.bpb.fat_sectors;
    printf(" root_dir at sector: %i (offset %#x)\n", root_dir_clus, root_dir_clus*512);
    fseek(f, root_dir_clus*512, 0);

    first_data_sector = bootsect.bpb.ressects +
                        bootsect.bpb.fat_count*bootsect.bpb.fat_sectors +
                        root_dir_sectors;
    printf(" first_data_sector : %i (offset %#x)\n", first_data_sector, first_data_sector*512);

    struct file_record root_dir[bootsect.bpb.root_fd_num-1];
    fread(root_dir, 1, sizeof(root_dir), f);


    /* Набор LFN-записей каталога FAT всегда должен быть
     * связан с обычной SFN-записью                      */
    for (i = 0; i < 16 /*bootsect.bpb.root_fd_num/2*/; i += 1) {
        putchar('\n');
        if ((0xff & root_dir[i].name[0]) == 0xe5 ||
            (0xff & root_dir[i].name[0]) == 0x05)
        {
            printf(" [*] record marked deleted\n");
        } else if ((0xff & root_dir[i].name[0]) == 0) {
            printf(" [*] record and the rest of directory marked deleted\n");
        } else {
            printf(" [+] record ok\n");
        }
        printf("  |  fst_clus: %#x\n", (root_dir[i].fst_clus_lo));

        switch (root_dir[i].attr) {
        case (ATTR_VOLUME_ID | ATTR_ARCHIVE):
            printf("  |  found volume label record\n");
            printf("  |  name: \"%.11s\"\n", root_dir[i].name);
            break;

        case (ATTR_RO | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID):
            if (opts.verbose) {
                printf("  |  lfn (ord %#x)\n   | \"", 0xff&((struct long_filename *)(&root_dir[i]))->LDIR_ord);
                print_lfn((struct long_filename *)&root_dir[i]);
                printf("\"\n");
            }
            break;

        case ATTR_DIRECTORY:
            printf("  |  directory\n");
            printf("  |  name: \"%.11s\"\n", root_dir[i].name);
            parse_dir(&root_dir[i]);
            break;

        default:
            print_file(&root_dir[i],
                       bootsect.bpb.sector_size*bootsect.bpb.sectincluster,
                       bootsect.bpb.sectincluster,
                       first_data_sector);
            break;
        }
    }

    errno = 0;
    free(fat);
    fclose(f);
    perror("fclose");
    return 0;
}