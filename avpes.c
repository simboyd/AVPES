// made by Sandro Jguburia for CS50 Final Project
// AVPES is a super duper primitive encryption suite
// that can do following things:
//
// 1) encrypt and decrypt a file by XORing bytes with a
// cryptographically-secure random number generated by
// the libsodium library.
//
// 2) encrypt and decrypt a file using a vigenere-like method
// where the key is provided by the user, which is then used
// for some simple byte manipulation.
//
// 3) zero-out a file and delete it. This irreversibly destroys all data
// contained within the file.
//
// 4) encrypt/decrypt data to/from an uncompressed 24-bit bitmap file

// usage examples:
// avpes --encdef myfile.txt
// avpes --decdef encrypted_myfile.txt keymap_myfile.txt
//
// avpes --encvig myfile.txt mykey.txt
// avpes --decvig myfile.txt mykey.txt
//
// avpes --zero myfile.txt
//
// avpes --encbmp myimage.bmp mydata.dat
// avpes --decbmp myimage.bmp 1000
//
// if you're a a recruiter or something, STOP! DO NOT GO FORWARD.
// (in case if you do, i'm better than this now. Much much better. I promise.)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <stdint.h>
#include <sodium.h>

typedef uint32_t DWORD; // 4 bytes, unsigned
typedef int32_t  LONG; // 4 bytes, signed
typedef uint16_t WORD; // 2 bytes, unsigned

typedef unsigned char uchar8;
typedef unsigned long uint32;
void encDef(const char *); // default encryption
void encVig(const char *, const char *); // vigenere-like encryption
void decDef(const char *, const char *); // default decryption
void decVig(const char *, const char *); // vigenere-like decryption
void encBmp(const char *, const char *);
void decBmp(const char *, const uint32);
uint32 fileSize(FILE *); // spits out filesize
uint32 progress(uint32, uint32, uint32); // percentage
void zero(const char *, const uint32); // zeroes out a file completely
void ask(const char *, const uint32);
void byteFormat(uchar8 *, uchar8);

#pragma pack(push, 1) // disabling structure padding
typedef struct
{
    WORD bfType; // signature[2]
    DWORD bfSize;
    WORD bfReserved1; // two 0x0s
    WORD bfReserved2; // two 0x0s
    DWORD bfOffBits;
} //__attribute__ ((__packed__)) // DO NOT PAD!
BITMAPFILEHEADER;
//#pragma pack(pop) // DO NOT PAD!

typedef struct
{
    DWORD biSize;
    LONG biWidth; // use this to calculate padding via magic formula
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount; // must be 24, otherwise error
    DWORD biCompression; // must be zero, otherwise error
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} //__attribute__ ((__packed__)) // DO NOT PAD!
BITMAPINFOHEADER;

int main(int argc, char *argv[])
{
    if(strcmp(argv[1], "--encdef") == 0)
    {
        if(argc != 3)
		{
			printf("Error: Must have two arguments.\n");
			exit(-22);
		}
		else
            encDef(argv[2]);
    }
    else if (strcmp(argv[1], "--encvig") == 0)
    {
        if(argc != 4)
		{
			printf("Error: Must have three arguments.\n");
			exit(-27);
		}
		else
            encVig(argv[2], argv[3]);
    }
    else if(strcmp(argv[1], "--decdef") == 0)
    {
		if(argc != 4)
		{
			printf("Error: Must have three arguments.\n");
			exit(-22);
		}
		else
			decDef(argv[2], argv[3]);
    }
    else if(strcmp(argv[1], "--decvig") == 0)
    {
		if(argc != 4)
		{
			printf("Error: Must have three arguments.\n");
			exit(-22);
		}
		else
			decVig(argv[2], argv[3]);
    }
    else if(strcmp(argv[1], "--encbmp") == 0)
    {
        if(argc != 4)
        {
            printf("Error: Must have three arguments.\n");
            exit(-69);
        }
        else
            encBmp(argv[2], argv[3]);
    }
    else if(strcmp(argv[1], "--decbmp") == 0)
    {
        if(argc != 4)
        {
            printf("Error: Must have three arguments.\n");
            exit(-70);
        }
        else
        {
            char *wtv;
            uint32 num = strtol(argv[3], &wtv, 10);
            decBmp(argv[2], num);
        }
    }
    else if(strcmp(argv[1], "--zero") == 0)
    {
        if(argc != 3)
        {
            printf("Error: Must have two arguments.\n");
            exit(-19);
        }
        else // a good example of how not to write code
        {
            FILE *fl = fopen(argv[2], "rb");
            if(!fl)
            {
                printf("Error: File not found;");
                exit(-70);
            }
            uint32 FlSize = fileSize(fl);
            fclose(fl);
            zero(argv[2], FlSize);
            printf("Would you like to delete it now? ");
            fflush(stdout);
            char usrInpt = getchar();
            if(usrInpt == 'y' || usrInpt == 'Y')
                remove(argv[2]);
        }
    }
    else
    {
        printf("%s%s%s%s%s%s%s%s",
        "Usage: avpes [mode] [file] [additional input (optional)]\n\t",
        "Modes:\n\n\t\t--encdef = default encryption\n\t\t",
        "--encvig = vigenere encryption (requires ASCII text file containing key)\n\t\t",
        "--decdef = default decryption (requires keymap file)\n\t\t",
        "--decvig = vigenere decryption (requires ASCII text file containing key)\n\t\t",
        "--zero   = zero-out mode; give it a filename and it will destroy its data.\n\t\t",
        "--encbmp = encode data of a file into the specified bitmap image.\n\t\t"
        "--decbmp = extract data from a bitmap image. Third argument should be the number of bytes to extract.\n");
        exit(-99);
    }

    return 0;
}

void encDef(const char *fname)
{
    if(sodium_init() != 0)
    {
        printf("Error initializing sodium.\n");
        exit(-8);
    }

    // cipherfile filename preparations
    char *outname       = (char *) calloc(strlen(fname) + strlen("keymap_") + 1, 
                        sizeof(char));
    char *encoutname    = (char *) calloc(strlen(fname) + strlen("encrypted_") + 1, 
                        sizeof(char));
    strcpy(outname, "keymap_");
	strcat(outname, fname);
    strcpy(encoutname, "encrypted_");
    strcat(encoutname, fname);

    // prepping plaintext and ciphertext files
    FILE *plainfile = fopen(fname, "rb");
    if(!plainfile)
    {
        free(outname);
        printf("Error: File not found.\n");
        exit(-98);
    }

    FILE *readyfile = fopen(encoutname, "wb");
    if(!readyfile)
    {
        free(outname);
		free(encoutname);
        fclose(plainfile);
        printf("Error: Encrypted file couldn't be created.\n");
        exit(-97);
    }

    FILE *cypherfile = fopen(outname, "wb");
    if(!cypherfile)
    {
        free(outname);
		free(encoutname);
        fclose(plainfile);
		fclose(readyfile);
        printf("Error: keymap file couldn't be created.\n");
        exit(-97);
    }
    
    const uint32 filesize   = fileSize(plainfile);
    uchar8 buffer           = 0;
    uchar8 key              = 0;
	uint32 speed            = 0; 
    uint32 unix             = (uint32) time(NULL);

	printf("Progress: [00.00%%]");
	fflush(stdout);

    // actual encryption happens here :3
    for(uint32 i = 1; i <= filesize; i++)
    {
        fread(&buffer, 1, 1, plainfile);
        key = randombytes_random();
        buffer = buffer ^ key; // XOR current byte with a random number
        fwrite(&buffer, 1, 1, readyfile); // write encrypted byte to the file
        fwrite(&key, 1, 1, cypherfile); // write random number to the keymap file
        speed++;
		if(unix < ((uint32) time(NULL)))
        {
			unix = progress((uint32) i, filesize, speed);
            speed = 0;
        }
    }

    fclose(plainfile);
    fclose(cypherfile);
    fclose(readyfile);
	printf("\rEncryption completed.             \nEncrypted file: %s\n", encoutname);
    printf("Keymap file: %s\n", outname);
    free(outname);
    free(encoutname);
	ask(fname, filesize);
}

void encVig(const char *fname, const char *keyname) // encode using vigenere cipher
{
	FILE *ufl = fopen(fname, "rb");
    if(!ufl)
    {
        printf("Unable to open file %s. Does it exist?\n", fname);
        exit(-64);
    }
    
    FILE *keyfl = fopen(keyname, "rb");
    if(!keyfl)
    {
        printf("Error opening your cipher file (%s).\n", keyname);
        fclose(ufl);
        exit(-29);
    }

    char *outname = calloc(strlen(fname) + strlen("encrypted_") + 1, sizeof(char));
    strcpy(outname, "encrypted_");
    strcat(outname, fname);
    
    FILE *efl = fopen(outname, "wb");
    if(!efl)
    {
        printf("Unable to create encrypted file (%s).\n", outname);
        free(outname);
        fclose(ufl);
        fclose(keyfl);
    }
    
    uint32 uflSize      = fileSize(ufl);
    uint32 keySize      = fileSize(keyfl);
    uint32 speed        = 0;
    uint32 unix         = (uint32) time(NULL);
    uchar8 byteFile     = 0x0;
    uchar8 byteCipher   = 0x0;
    uchar8 byteEnc      = 0x0;

    for(uint32 i = 1, curpos = 0; i <= uflSize; i++)
    {
        fread(&byteFile, 1, 1, ufl);
        do{
            if(ftell(keyfl) == keySize)
                fseek(keyfl, 0, SEEK_SET);
            fscanf(keyfl, "%c", &byteCipher);
        }while(isalpha(byteCipher) == 0);
        
        byteEnc = byteFile ^ byteCipher;
        fwrite(&byteEnc, 1, 1, efl);

        speed++;
        if(unix < (uint32) time(NULL))
        {
            unix = progress(i, uflSize, speed);
            speed = 0;
        }
    }

    fclose(ufl);
    fclose(keyfl);
    fclose(efl);
    free(outname);
    printf("\rEncryption completed.                   \n");
    ask(fname, uflSize);
}

void decDef(const char *fname, const char *keyname) //default decode
{
    FILE *encryptedFile = fopen(fname, "rb");
    if(!encryptedFile)
    {
        printf("Couldn't open file for decryption. Does it exist?\n");
        exit(-32);
    }
    FILE *keymapFile = fopen(keyname, "rb");
    if(!keymapFile)
    {
        fclose(encryptedFile);
        printf("Couldn't open keymap file for decryption. Does it exist?\n");
        exit(-31);
    }

    //preparing decrypted filename
    char *resultName =  (char *) calloc(strlen(fname) + strlen("decrypted_") + 1, 
                        sizeof(char));
    strcpy(resultName, "decrypted_");
    strcat(resultName, fname);

    FILE *decryptedFile = fopen(resultName, "wb");
    if(!decryptedFile)
    {
        fclose(encryptedFile);
        fclose(keymapFile);
        free(resultName);
        printf("Couldn't create decrypted file.\n");
        exit(-30);
    }
 
    const uint32 encFile    = fileSize(encryptedFile);
    const uint32 keyFile    = fileSize(keymapFile);
    uint32 unix             = (uint32) time(NULL);
    uint32 speed            = 0;
    uchar8 bufferEnc        = 0x0;
    uchar8 bufferKey        = 0x0; 
    uchar8 readyByte        = 0x0;

    if(encFile != keyFile)
    {
        printf("%s%s", 
        "Error: Your keymap file doesn't belong to your encrypted file.", 
        "Decryption cannot continue.\n");
        fclose(encryptedFile);
        fclose(keymapFile);
        fclose(decryptedFile);
        free(resultName);
        exit(-42);
    }

	printf("Progress: [00.00%%], X BT/s");
	fflush(stdout);
    for(uint32 i = 1; i <= encFile; i++) // actual decryption happens here
    {
        fread(&bufferEnc, 1, 1, encryptedFile);
        fread(&bufferKey, 1, 1, keymapFile);
        readyByte = bufferEnc ^ bufferKey;
        fwrite(&readyByte, 1, 1, decryptedFile);
        speed++;
		if(unix < ((uint32) time(NULL)))
        {
			unix = progress((uint32) i, encFile, speed);
            speed = 0;
        }
    }
    printf("\rFile decrypted successfully.           \nDecrypted file: %s\n", 
            resultName);

    fclose(encryptedFile);
    fclose(keymapFile);
    fclose(decryptedFile);
    free(resultName);
	
    char usrInpt;
	printf("Delete encrypted file (%s)? (Y/N) ", fname);
    fflush(stdout);
	usrInpt = getchar();
	if(usrInpt == 'y' || usrInpt == 'Y')
		remove(fname);
	fflush(stdin);
	printf("Delete keymap file (%s)? (Y/N) ", keyname);
    fflush(stdout);
	usrInpt = getchar();
	if(usrInpt == 'y' || usrInpt == 'Y')
		remove(keyname);
    printf("All done.\n");
}

void decVig(const char *fname, const char *keyname) //decode using vigenere cypher
{
	FILE *efl = fopen(fname, "rb");
    if(!efl)
    {
        printf("Error opening encrypted file (%s). Does it exist?\n", fname);
        exit(-12);
    }
    FILE *keyfl = fopen(keyname, "rb");
    if(!keyfl)
    {
        fclose(efl);
        printf("Error opening key file (%s). Does it exist?\n", keyname);
        exit(-9);
    }

    char *outname = calloc(strlen(fname) + strlen("decrypted_") + 1,    
                    sizeof(char));
    strcpy(outname, "decrypted_");
    strcat(outname, fname);

    FILE *outfl = fopen(outname, "wb");
    if(!outfl)
    {
        fclose(efl);
        fclose(keyfl);
        free(outname);
        printf("Error creating decrypted file (%s).\n", outname);
        exit(-13);
    }

    uint32 encsize      = fileSize(efl);
    uint32 keySize      = fileSize(keyfl);
    uint32 speed        = 0;
    uint32 unix         = (uint32) time(NULL);
    uchar8 byteFile     = 0x0;
    uchar8 byteCipher   = 0x0;
    uchar8 byteDec      = 0x0;

    for(uint32 i = 1; i <= encsize; i++)
    {
        fread(&byteFile, 1, 1, efl);
        do{
            if(ftell(keyfl) == keySize)
                fseek(keyfl, 0, SEEK_SET);
            fscanf(keyfl, "%c", &byteCipher);
        }while(isalpha(byteCipher) == 0);
        
        byteDec = byteFile ^ byteCipher;
        fwrite(&byteDec, 1, 1, outfl);

        speed++;
        if(unix < (uint32) time(NULL))
        {
            unix = progress(i, encsize, speed);
            speed = 0;
        }
    }

    fclose(efl);
    fclose(keyfl);
    fclose(outfl);
    free(outname);

    printf("\rFile decrypted successfully.             \n");
    printf("Delete the encrypted file (%s)? (Y/N) ", fname);
    fflush(stdout);
    char usrInpt = getchar();
    if(usrInpt == 'y' || usrInpt == 'Y')
        remove(fname);
    printf("Would you like to delete the key file as well (%s)? (Y/N) ", keyname);
    fflush(stdout);
    fflush(stdin);
    usrInpt = getchar();
    if(usrInpt == 'y' || usrInpt == 'Y')
        remove(keyname);
    printf("\nAll done.\n");
}

uint32 fileSize(FILE *fl) //find out filesize of fl
{
    uint32 flsize = 0;
    fseek(fl, 0, SEEK_END);
    flsize = ftell(fl);
    fseek(fl, 0, SEEK_SET);
    return flsize;
}

uint32 progress(uint32 current, uint32 total, uint32 speed)
{
	float percentage = ((float) current / (float) total) * 100.0;
	printf("\rProgress: [%05.2f%%]", percentage);
    if(speed >= 1024 && speed < 1048576)
        printf(", %.2lf KB/s           ", speed/1024.0);
    else if(speed >= 1048576 && speed < 1073741824)
        printf(", %.2lf MB/s           ", speed/1048576.0);
    else if(speed >= 1073741824)
        printf(", %.2lf GB/s           ", speed/1073741824.0);
    else
        printf(", %.2lf BT/s           ", speed);
	fflush(stdout);
	return (uint32) time(NULL);
}

void zero(const char *filename, const uint32 filesizeX)
{
    FILE *fl = fopen(filename, "r+b");
    if(!fl)
    {
        printf("Error opening file %s\n", filename);
        exit(-20);
    }

    uchar8 byte     = 0x0;
    uint32 speed    = 0;
    uint32 unix     = (uint32) time(NULL);

    printf("\rProgress: [00.00%%]");
    fflush(stdout);
    for(uint32 i = 1; i <= filesizeX; i++)
    {
        fwrite(&byte, 1, 1, fl);
        speed++;
        if(unix < (uint32) time(NULL))
        {
            unix = progress(i, filesizeX, speed);
            speed = 0;
        }
    }

    fclose(fl);
    printf("\r%s has been zeroed out successfully.\n", filename);
}

void ask(const char *fname, const uint32 filesize)
{

    printf("\rWould you like to delete the unencrypted file (%s)? (Y/N) ", 
            fname);
    fflush(stdout);
    char usrInpt = getchar();
    fflush(stdin);
    if(usrInpt == 'y' || usrInpt == 'Y')
    {
        printf("Would you like to zero it out before deleting? (Y/N) ");
        fflush(stdout);
        usrInpt = getchar();
        if(usrInpt == 'y' || usrInpt == 'Y')
        {
            zero(fname, filesize);
            if(!remove(fname))
                printf("File successfully deleted.\n");
            else
                printf("Error deleting file.\n");
        }
        else
        {
            if(!remove(fname))
                printf("File successfully deleted.\n");
            else
                printf("Error deleting file.\n");
        }
    }
    else
        printf("All done.\n");
}

void encBmp(const char *bmpname, const char *plain)
{
    FILE *bmp = fopen(bmpname, "rb");
    if(!bmp)
    {
        printf("Couldn't open file %s. Does it exist?\n", bmpname);
        exit(-4);
    }

    FILE *text = fopen(plain, "rb");
    if(!text)
    {
        fclose(bmp);
        printf("Couldn't open unencrypted file %s. Does it exist?\n", plain);
        exit(-17);
    }

    // first, we check if we can actually work with this file

    uint32 sizeText = fileSize(text);
    BITMAPFILEHEADER fhead;
    BITMAPINFOHEADER ihead;
    fread(&fhead, sizeof(BITMAPFILEHEADER), 1, bmp);
    fread(&ihead, sizeof(BITMAPINFOHEADER), 1, bmp);

    if(fhead.bfType != 0x4d42)
    {
        printf("The file provided doesn't seem to be a proper bitmap file.\n");
        fclose(bmp);
        fclose(text);
        exit(-27);
    }

    if(ihead.biBitCount != 24)
    {
        printf("%s is not a 24-bit bitmap image.\n", bmpname);
        fclose(bmp);
        fclose(text);
        exit(-26);
    }
    
    if(ihead.biCompression != 0)
    {
        printf("%s is not an uncompressed bitmap file.\n", bmpname);
        fclose(bmp);
        fclose(text);
        exit(-56);
    }

    if(sizeText > ((fhead.bfSize - 54) / 4) - 9)
    {
        printf("This bitmap image is too small to encode your data in it.\n");
        fclose(bmp);
        fclose(text);
        exit(-44);
    }

    // now we know the bitmap is usable, so we're good to go


    char *outname = (char *) calloc(strlen(bmpname) + strlen("encrypted_") + 1, 
                    sizeof(char));
    strcpy(outname, "encrypted_");
    strcat(outname, bmpname);
    FILE *outfile = fopen(outname, "wb");
    if(!outfile)
    {
        printf("Unable to create encrypted bitmap.\n");
        exit(-61);
    }

    fwrite(&fhead, sizeof(fhead), 1, outfile);
    fwrite(&ihead, sizeof(ihead), 1, outfile);

    short padding   = (4 - (ihead.biWidth * 3) % 4) % 4; // pure magic; 3 is sizeof(RGB)
    uint32 fsize    = fileSize(bmp);
    uint32 tsize    = fileSize(text);
    uchar8 pxlbyte  = 0x0;
    uchar8 txtbyte  = 0x0;
    uchar8 modpxl   = 0x0;
    uchar8 pxlbytef[9];
    uchar8 txtbytef[9];
    uint32 i, iterator;
    int padcheckB, padcheckA;
    uchar8 pad = 0x0;
    char *wtv;
    fseek(bmp, 54, SEEK_SET); // skip headers


    //magic happens in this loop
    for(i = 0, padcheckB = 1, padcheckA = 1, iterator = 0; 
        i < tsize*4; i++, iterator++, padcheckA++)
    {
        if(i % 4 == 0)
        {
            fread(&txtbyte, 1, 1, text);
            byteFormat(txtbytef, txtbyte);
            padcheckB++;
            iterator = 0;
        }

        fread(&pxlbyte, 1, 1, bmp);
        byteFormat(pxlbytef, pxlbyte);

        pxlbytef[6] = txtbytef[iterator];
        pxlbytef[7] = txtbytef[++iterator];

        modpxl = strtol(pxlbytef, &wtv, 2);
        fwrite(&modpxl, 1, 1, outfile);

        if(padcheckB == ihead.biWidth * 3)
        {
            fseek(bmp, padding, SEEK_CUR); // skip padding
            padcheckB = 1;
        }

        if(padcheckA == ihead.biWidth * 3)
        {
            fwrite(&pad, 1, padding, outfile);
            padcheckA = 1;
        }
    }

    while(fread(&pxlbyte, 1, 1, bmp) == 1) // do the rest of it
        fwrite(&pxlbyte, 1, 1, outfile);

    printf("%d bytes have been altered and written to %s.\n", tsize, outname);

    free(outname);
    fclose(outfile);
    fclose(bmp);
    fclose(text);
}

void decBmp(const char *fname, const uint32 amount)
{
    FILE *bmp = fopen(fname, "rb");
    if(!bmp)
    {
        printf("Couldn't open bmp file.\n");
        exit(-35);
    }

    BITMAPFILEHEADER fhead;
    BITMAPINFOHEADER ihead;
    fread(&fhead, sizeof(BITMAPFILEHEADER), 1, bmp);
    fread(&ihead, sizeof(BITMAPINFOHEADER), 1, bmp);
    fseek(bmp, 54, SEEK_SET); // just in case, i dunno

    if(amount > ((fhead.bfSize / 4) - 54))
    {
        printf("The number of bytes to extract is too large.\n");
        fclose(bmp);
        exit(-355);
    }

    if(fhead.bfType != 0x4d42)
    {
        printf("The file provided doesn't seem to be a proper bitmap file.\n");
        fclose(bmp);
        exit(-337);
    }

    if(ihead.biBitCount != 24)
    {
        printf("%s is not a 24-bit bitmap image.\n", fname);
        fclose(bmp);
        exit(-262);
    }
    
    if(ihead.biCompression != 0)
    {
        printf("%s is not an uncompressed bitmap file.\n", fname);
        fclose(bmp);
        exit(-546);
    }

    // we got here so everything is ok
    char *outname = (char *) calloc(strlen(fname) + strlen("decrypted_") + 1, 
                    sizeof(char));
    strcpy(outname, "decrypted_");
    strcat(outname, fname);

    FILE *output = fopen(outname, "wb");
    if(!output)
    {
        printf("Unable to create file for output.\n");
        fclose(bmp);
        free(outname);
        exit(-211);
    }

    short padding   = (4 - (ihead.biWidth * 3) % 4) % 4;
    short i = 0;
    char *wtv;
    uchar8 byte;
    uchar8 readybyte;
    uchar8 bytef[9];
    uchar8 readybytef[9];
    readybytef[8] = '\0';
    bytef[8] = '\0';
    double general = 0.0;
    uchar8 pad = 0x0;
    long int amountx = 0;

    //magic happens here
    while(amountx < amount*4)
    {
        fread(&byte, 1, 1, bmp);
        byteFormat(bytef, byte);

        readybytef[i]   = bytef[6];
        readybytef[i+1] = bytef[7];
        
        i += 2;
        
        if(i > 7)
        {
            readybyte = (uchar8) strtol(readybytef, &wtv, 2);
            fwrite(&readybyte, 1, 1, output);
            i = 0;
        }
        
        general++;
        amountx++;

        if(general / 3.0 == (double) ihead.biWidth)
        {
            fseek(bmp, padding, SEEK_CUR);
            general = 0.0;
        }
    }

    printf("%ld bytes have been extracted into the file %s.\n", amount, outname);

    free(outname);
    fclose(output);
    fclose(bmp);
}

void byteFormat(uchar8 *str, uchar8 byte)
{
    uchar8 temp[9];
    int j;
    itoa((int) byte, temp, 2);
    for(j = 0; j < 8-strlen(temp); j++)
        str[j] = '0';
    str[j] = '\0';
    strcat(str, temp);
}
