**AVPES** is a very primitive encryption suite that does several things:

1. Encrypts a file using the default built-in encryption                 (--encdef)
2. Decrypts a file using the default built-in decryption                 (--decdef)
3. Encrypts a file using a vigenere-like method of encryption            (--encvig)
4. Decrypts a file using a vigenere-like method of decryption            (--decvig)
5. Zeroes out a file for irreversible deletion                           (--zero) 
6. Inserts data into an uncompressed 24-bit bitmap image                 (--encbmp)
7. Extracts data from an uncompressed 24-bit bitmap image                (--decbmp)

## 1.
*Example: `avpes.exe --encdef myfile.dat`*

Default encryption method entails encrypting individual bytes of a file. In order to do this, it utilizes the [Sodium library](https://libsodium.org) for generating random numbers, by which it modifies the bytes themselves. Ultimately, this method creates two new files: an encrypted file and a keymap file. The keymap file contains the randomly generated numbers used for encryption. The idea is to keep these two files separate and use them together in AVPES for decryption. It then, optionally, deletes and zeroes out the original (unencrypted) file.

## 2. 
*Example: `avpes.exe --decdef my_encrypted_file.dat keymap_myfile.dat`*

Default decryption uses the data from the keymap file for modifying (decrypting) the bytes from the encrypted file. It stores the modified bytes in a new file called decrypted_[yourfile]. This method also deletes your keymap and encrypted files (optional).

## 3. 
*Example: `avpes.exe --encvig myfile.dat mykey.txt`*

Vigenere-like encryption uses user-provided bytes for encryption instead of random bytes/numbers. The "keyfile" can be any kind of file (not just a text file), since AVPES uses the bytes themselves for this kind of encryption rather than just characters. It creates the new encrypted file and optionally deletes and zeroes out the unencrypted file.

## 4.
*Example: `avpes.exe --decvig my_encrypted_file.dat mykey.txt`*

This simply decrypts your file using the given keyfile. See 3. for more info. Optionally deletes your keymap and encrypted files.

## 5.
*Example: `avpes.exe --zero myfile.dat`*

This simply zeroes out a file, making it impossible to recover the data. This is arguably the only sane, non-gimmicky part of this program that may actually be useful to someone.

## 6.
*Example: `avpes.exe --encbmp myimage.bmp mydata.dat`*

Inserts data (bytes) of a file into a bitmap image. The bitmap image has to be uncompressed and 24-bit. It chucks out a new image that has new data encrypted into it (encrypted_[yourbmp.bmp]). It stores the new encrypted data into the last two bits of every pixel byte. After completion, it prints out the number of bytes that have been altered. You must use this number in 7. for recovering the data. See below.

## 7.
*Example: `avpes.exe --decbmp my_image_that_has_data_in_it.bmp 100000`*

Extracts data from a bmp image. The third argument should be the number of bytes to extract (this number is spat out by AVPES after insertion in #6, see above).


P.S. it uses libsodium.

P.P.S. bmp stuff doesn't seem to work that well with BMPs over 20MB. I really can't be bothered to fix it.

###### Made by Sandro (@simboyd)
