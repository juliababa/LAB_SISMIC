; Código Feito no IAR
; Aluno(a): Júlia Yuri Garcia Baba
; Matrícula: 190057921 - TURMA A

#include "msp430.h"                     ; #define controlled include file

        NAME    main                    ; module name

        PUBLIC  main                    ; make the main label vissible
                                        ; outside this module
        ORG     0FFFEh
        DC16    init                    ; set reset vector to 'init' label

        RSEG    CSTACK                  ; pre-declaration of segment
        RSEG    CODE                    ; place program in 'CODE' segment

init:   MOV     #SFE(CSTACK), SP        ; set up stack

main:   NOP                             ; main program
        MOV.W   #WDTPW+WDTHOLD,&WDTCTL  ; Stop watchdog timer
        
VISTO1:
           MOV #MSG,R5
           MOV #GSM,R6
           CALL #ENIGMA ;Cifrar

           MOV #GSM,R5
           MOV #DCF,R6
           CALL #ENIGMA ;Cifrar
           JMP $
           NOP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;                  Inicio switch case              ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ENIGMA:
            mov.w #CHAVE, R8 ;armaneza o endereço da chave 
            mov.b 1(R8), R12 ;valor configuração
            mov.b 0(R8), R9
            add.w R9, PC     ;intrução para pular para as proximas linhas
            jmp case_r1
            jmp case_r2
            jmp case_r3
            jmp case_r4
            jmp case_r5

case_r1:
            mov.w #RT1, R10
            jmp final_rotor
case_r2:
            mov.w #RT2, R10
            jmp final_rotor
case_r3:
            mov.w #RT3, R10
            jmp final_rotor
case_r4:
            mov.w #RT4, R10
            jmp final_rotor
case_r5:
            mov.w #RT5, R10
            
final_rotor:

            mov.b 2(R8), R9 ;valor refletor
            add.w R9, PC    ;intrução para pular para as proximas linhas
            jmp case_ref1
            jmp case_ref2
            jmp case_ref3

case_ref1:
            mov.w #RF1, R11
            jmp loop
case_ref2:
            mov.w #RF2, R11
            jmp loop
case_ref3:
            mov.w #RF3, R11
 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;                 Fim switch case                  ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

loop:
            mov.b @R5, R15 ;pega a primeira letra
            inc.w R5

            tst.b R15       ;testa para ver se é zero
            jz final_string ;jump if zero

            cmp.b #';', R15 ;se não esta entre ; e Z apenas repete
            jlo pula_caracter
            cmp.b #'Z', R15
            jhs pula_caracter

            call #rotor
            call #refletor
            call #caminho_rev_rotor

pula_caracter:
            mov.b R15, 0(R6) ;copia 
            inc.w R6 ;vai para a próxima letra
            jmp loop

final_string:
            ret

rotor:
            push.w R10
            sub.w #';', R15
            add.w R12, R15

            cmp.w #'Z', R15
            jlo posi_rotor
            sub.w #'Z', R15

posi_rotor:                     ;percorre o vetor
            jz subst_caracter
            inc.w R10
            dec.w R15
            jmp posi_rotor
subst_caracter:                 ;volta para o padrão ASCII

            mov.b 0(R10), R15
            add.b #';', R15

            pop.w R10
            ret

refletor:
            push.w R11
            sub.w #';', R15
posi_refletor:
            jz subst_caracter_refletor
            inc.w R11
            dec.w R15
            jmp posi_refletor
subst_caracter_refletor:

            mov.b 0(R11), R15
            add.b #';', R15
            pop.w R11
            ret

caminho_rev_rotor:
            sub.w #';', R15
            mov.w #0, R7
            push.w R10
main_loop:
            cmp.b 0(R10), R15
            jeq posicao
            inc.w R7
            inc.w R10
            jmp main_loop
posicao:
            mov.b R7, R15
            sub.w R12, R15

            cmp.w #0, R15
            jhs ajuste_limite

            add.w #'Z', R15
ajuste_limite:
            add.w #';', R15
            pop.w R10
            ret

                        
 aseg 0x2400
CHAVE db 1, 1, 2
RT_TAM equ 32

MSG  db "UMA NOITE DESTAS, VINDO DA CIDADE PARA O ENGENHO NOVO, ENCONTREI NO TREM DA CENTRAL UM RAPAZ AQUI DO BAIRRO, QUE EU CONHECO DE VISTA E DE CHAPEU.", 0 ;Dom Casmurro
GSM  db "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", 0
DCF  db "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", 0

;Rotores com 32 posições

RT1  db 13, 23, 0, 9, 4, 2, 5, 11, 12, 17, 21, 6, 28, 25, 30, 10, 22, 1, 3, 26, 24, 31, 8, 14, 29, 15, 18, 16, 19, 7, 27, 20

RT2  db 6, 24, 2, 8, 25, 20, 16, 29, 23, 0, 7, 19, 30, 17, 12, 15, 5, 4, 26, 10, 11, 18, 28, 27, 14, 9, 13, 1, 21, 31, 22, 3

RT3  db 6, 15, 23, 7, 27, 13, 19, 3, 16, 4, 17, 20, 24, 25, 0, 10, 30, 26, 22, 1, 8, 11, 14, 31, 9, 28, 5, 18, 12, 2, 29, 21

RT4  db 15, 16, 5, 18, 31, 26, 19, 28, 1, 2, 14, 12, 24, 20, 21, 0, 11, 23, 4, 10, 7, 3, 25, 29, 27, 8, 17, 6, 9, 13, 22, 30

RT5  db 13, 25, 1, 26, 6, 12, 9, 2, 28, 11, 16, 15, 4, 8, 3, 31, 5, 18, 23, 17, 24, 27, 0, 22, 29, 19, 7, 10, 14, 21, 20, 30

;Refletores com 32 posições
RF1  db 26, 23, 31, 9, 29, 20, 16, 11, 27, 3, 14, 7, 21, 28, 10, 25, 6, 22, 24, 30, 5, 12, 17, 1, 18, 15, 0, 8, 13, 4, 19, 2

RF2  db 20, 29, 8, 9, 23, 27, 21, 11, 2, 3, 25, 7, 13, 12, 22, 16, 15, 28, 30, 26, 0, 6, 14, 4, 31, 10, 19, 5, 17, 1, 18, 24

RF3  db 14, 30, 7, 5, 15, 3, 18, 2, 23, 17, 29, 28, 25, 27, 0, 4, 19, 9, 6, 16, 26, 22, 21, 8, 31, 12, 20, 13, 11, 10, 1, 24

END