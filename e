[33mcommit 78352533c1602fd155c31553c69316eb47812c97[m[33m ([m[1;36mHEAD -> [m[1;32mmain[m[33m, [m[1;31morigin/main[m[33m, [m[1;31morigin/HEAD[m[33m)[m
Author: Mati Sicher <sicher2001@gmail.com>
Date:   Sat Dec 16 08:52:24 2023 +0000

    solucion error de locks en archivos. se borraba el nombre

[1mdiff --git a/filesystem/fs/bloques.dat b/filesystem/fs/bloques.dat[m
[1mindex 544ff40..e6dc196 100644[m
Binary files a/filesystem/fs/bloques.dat and b/filesystem/fs/bloques.dat differ
[1mdiff --git a/filesystem/fs/fat.dat b/filesystem/fs/fat.dat[m
[1mindex 2f56ff5..bb4b007 100644[m
Binary files a/filesystem/fs/fat.dat and b/filesystem/fs/fat.dat differ
[1mdiff --git a/filesystem/fs/fcbs/ArchivoRandom.fcb b/filesystem/fs/fcbs/ArchivoRandom.fcb[m
[1mdeleted file mode 100644[m
[1mindex 62c73bf..0000000[m
[1m--- a/filesystem/fs/fcbs/ArchivoRandom.fcb[m
[1m+++ /dev/null[m
[36m@@ -1,3 +0,0 @@[m
[31m-NOMBRE_ARCHIVO=ArchivoRandom[m
[31m-BLOQUE_INICIAL=1[m
[31m-TAMANIO_ARCHIVO=512[m
[1mdiff --git a/filesystem/fs/fcbs/NOTAS2C2023.fcb b/filesystem/fs/fcbs/NOTAS2C2023.fcb[m
[1mnew file mode 100644[m
[1mindex 0000000..f6aac04[m
[1m--- /dev/null[m
[1m+++ b/filesystem/fs/fcbs/NOTAS2C2023.fcb[m
[36m@@ -0,0 +1,3 @@[m
[32m+[m[32mNOMBRE_ARCHIVO=NOTAS2C2023[m
[32m+[m[32mBLOQUE_INICIAL=[m[41m [m
[32m+[m[32mTAMANIO_ARCHIVO=0[m
[1mdiff --git a/filesystem/fs/fcbs/consolas.fcb b/filesystem/fs/fcbs/consolas.fcb[m
[1mnew file mode 100644[m
[1mindex 0000000..2243829[m
[1m--- /dev/null[m
[1m+++ b/filesystem/fs/fcbs/consolas.fcb[m
[36m@@ -0,0 +1,3 @@[m
[32m+[m[32mNOMBRE_ARCHIVO=consolas[m
[32m+[m[32mBLOQUE_INICIAL=1[m
[32m+[m[32mTAMANIO_ARCHIVO=64[m
[1mdiff --git a/filesystem/fs/fcbs/fibonacci.fcb b/filesystem/fs/fcbs/fibonacci.fcb[m
[1mnew file mode 100644[m
[1mindex 0000000..51594ce[m
[1m--- /dev/null[m
[1m+++ b/filesystem/fs/fcbs/fibonacci.fcb[m
[36m@@ -0,0 +1,3 @@[m
[32m+[m[32mNOMBRE_ARCHIVO=fibonacci[m
[32m+[m[32mBLOQUE_INICIAL=5[m
[32m+[m[32mTAMANIO_ARCHIVO=160[m
[1mdiff --git a/filesystem/scripts/base.sh b/filesystem/scripts/base.sh[m
[1mindex 6c1b118..48c663b 100644[m
[1m--- a/filesystem/scripts/base.sh[m
[1m+++ b/filesystem/scripts/base.sh[m
[36m@@ -1,4 +1,12 @@[m
 #!/bin/bash[m
 [m
[32m+[m[32m@echo off[m
[32m+[m
[32m+[m[32mrm ./fs/bloques.dat[m
[32m+[m[32mrm ./fs/fat.dat[m
[32m+[m[32mrm ./fs/fcbs/*.fcb[m
[32m+[m
[32m+[m[32mecho "Eliminación completada."[m
[32m+[m
 make[m
 ./bin/filesystem.out ./config/base.config[m
\ No newline at end of file[m
[1mdiff --git a/filesystem/scripts/estres.sh b/filesystem/scripts/estres.sh[m
[1mindex 8d3ad47..b31e1df 100644[m
[1m--- a/filesystem/scripts/estres.sh[m
[1m+++ b/filesystem/scripts/estres.sh[m
[36m@@ -1,4 +1,12 @@[m
 #!/bin/bash[m
 [m
[32m+[m[32m@echo off[m
[32m+[m
[32m+[m[32mrm ./fs/bloques.dat[m
[32m+[m[32mrm ./fs/fat.dat[m
[32m+[m[32mrm ./fs/fcbs/*.fcb[m
[32m+[m
[32m+[m[32mecho "Eliminación completada."[m
[32m+[m
 make[m
 ./bin/filesystem.out ./config/estres.config[m
\ No newline at end of file[m
[1mdiff --git a/filesystem/scripts/fs.sh b/filesystem/scripts/fs.sh[m
[1mindex 03d2bd2..0e57826 100644[m
[1m--- a/filesystem/scripts/fs.sh[m
[1m+++ b/filesystem/scripts/fs.sh[m
[36m@@ -1,4 +1,12 @@[m
 #!/bin/bash[m
 [m
[32m+[m[32m@echo off[m
[32m+[m
[32m+[m[32mrm ./fs/bloques.dat[m
[32m+[m[32mrm ./fs/fat.dat[m
[32m+[m[32mrm ./fs/fcbs/*.fcb[m
[32m+[m
[32m+[m[32mecho "Eliminación completada."[m
[32m+[m
 make[m
 ./bin/filesystem.out ./config/fs.config[m
\ No newline at end of file[m
[1mdiff --git a/filesystem/scripts/integral.sh b/filesystem/scripts/integral.sh[m
[1mindex d01d305..4e456a0 100644[m
[1m--- a/filesystem/scripts/integral.sh[m
[1m+++ b/filesystem/scripts/integral.sh[m
[36m@@ -1,4 +1,12 @@[m
 #!/bin/bash[m
 [m
[32m+[m[32m@echo off[m
[32m+[m
[32m+[m[32mrm ./fs/bloques.dat[m
[32m+[m[32mrm ./fs/fat.dat[m
[32m+[m[32mrm ./fs/fcbs/*.fcb[m
[32m+[m
[32m+[m[32mecho "Eliminación completada."[m
[32m+[m
 make[m
 ./bin/filesystem.out ./config/integral.config[m
\ No newline at end of file[m
[1mdiff --git a/filesystem/scripts/memoria.sh b/filesystem/scripts/memoria.sh[m
[1mindex 23b68d2..ba21818 100644[m
[1m--- a/filesystem/scripts/memoria.sh[m
[1m+++ b/filesystem/scripts/memoria.sh[m
[36m@@ -1,4 +1,12 @@[m
 #!/bin/bash[m
 [m
[32m+[m[32m@echo off[m
[32m+[m
[32m+[m[32mrm ./fs/bloques.dat[m
[32m+[m[32mrm ./fs/fat.dat[m
[32m+[m[32mrm ./fs/fcbs/*.fcb[m
[32m+[m
[32m+[m[32mecho "Eliminación completada."[m
[32m+[m
 make[m
 ./bin/filesystem.out ./config/memoria.config[m
\ No newline at end of file[m
[1mdiff --git a/filesystem/scripts/recursos.sh b/filesystem/scripts/recursos.sh[m
[1mindex c4df7c9..0cbe6fc 100644[m
[1m--- a/filesystem/scripts/recursos.sh[m
[1m+++ b/filesystem/scripts/recursos.sh[m
[36m@@ -1,4 +1,12 @@[m
 #!/bin/bash[m
 [m
[32m+[m[32m@echo off[m
[32m+[m
[32m+[m[32mrm ./fs/bloques.dat[m
[32m+[m[32mrm ./fs/fat.dat[m
[32m+[m[32mrm ./fs/fcbs/*.fcb[m
[32m+[m
[32m+[m[32mecho "Eliminación completada."[m
[32m+[m
 make[m
 ./bin/filesystem.out ./config/recursos.config[m
\ No newline at end of file[m
[1mdiff --git a/kernel/.vscode/launch.json b/kernel/.vscode/launch.json[m
[1mindex 1825f8d..8ab9c7e 100644[m
[1m--- a/kernel/.vscode/launch.json[m
[1m+++ b/kernel/.vscode/launch.json[m
[36m@@ -10,7 +10,7 @@[m
       "request": "launch",[m
       "program": "${workspaceFolder}/bin/${workspaceFolderBasename}.out",[m
       "args": [[m
[31m-        "./config/estres.config"[m
[32m+[m[32m        "./config/fs.config"[m
         // TODO: Agregar los argumentos que se necesiten[m
       ],[m
       "stopAtEntry": false,[m
[1mdiff --git a/kernel/src/main.c b/kernel/src/main.c[m
[1mindex df6c0b6..5b4652a 100644[m
[1m--- a/kernel/src/main.c[m
[1m+++ b/kernel/src/main.c[m
[36m@@ -682,7 +682,7 @@[m [mt_archivo* crear_archivo_general(char* nombre_archivo){[m
 	t_archivo* archivo = malloc(sizeof(t_archivo));[m
 	archivo->nombre_archivo = nombre_archivo;[m
 	archivo->bloqueados_archivo = queue_create();[m
[31m-    archivo->modo_apertura = malloc(2);[m
[32m+[m[32m    //archivo->modo_apertura = malloc(2);[m
     archivo->modo_apertura = "";[m
     archivo->abierto_w = 0;[m
     archivo->cant_abierto_r = 0;[m
[36m@@ -694,7 +694,7 @@[m [mt_archivo_proceso* crear_archivo_proceso(char* nombre_archivo, char* modo_apertu[m
 	t_archivo_proceso* archivo = malloc(sizeof(t_archivo_proceso));[m
 	archivo->nombre_archivo = nombre_archivo;[m
 	archivo->puntero = 0;[m
[31m-    archivo->modo_apertura = malloc(2);[m
[32m+[m[32m    //archivo->modo_apertura = malloc(2);[m
     archivo->modo_apertura = modo_apertura;[m
 	return archivo;[m
 }[m
[1mdiff --git a/utils/src/protocolo/protocolo.c b/utils/src/protocolo/protocolo.c[m
[1mindex 1a23f64..e57d2d8 100644[m
[1m--- a/utils/src/protocolo/protocolo.c[m
[1m+++ b/utils/src/protocolo/protocolo.c[m
[36m@@ -232,7 +232,7 @@[m [mt_list* recv_archivos(t_log* logger, int fd_modulo) {[m
 }[m
 [m
 void archivo_destroyer(t_archivo* archivo) {[m
[31m-    free(archivo->nombre_archivo);[m
[32m+[m[32m    //free(archivo->nombre_archivo);[m
     free(archivo);[m
 }[m
 [m
