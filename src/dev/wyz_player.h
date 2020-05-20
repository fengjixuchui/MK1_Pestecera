#asm
	; CPC PSG proPLAYER V 0.47c - WYZ 19.03.2016
	; (WYZTracker 2.0 o superior)

	; ENSAMBLAR CON AsMSX 

	; LOS DATOS QUE HAY QUE VARIAR :
	; * BUFFER DE SONIDO DONDE SE DECODIFICA TOTALMENTE EL ARCHIVO MUS
	; * N� DE CANCION. 
	; * TABLA DE CANCIONES
	;______________________________________________________

	.WYZPLAYER_INIT
		CALL PLAYER_OFF

		LD DE, #0020 		; No. BYTES RESERVADOS POR CANAL
		
		LD HL, BASE_WYZ 	;* RESERVAR MEMORIA PARA BUFFER DE SONIDO!!!!!
		LD (CANAL_A), HL
		ADD HL, DE
		LD (CANAL_B), HL
		ADD HL, DE
		LD (CANAL_C), HL
		ADD HL, DE
		LD (CANAL_P), HL
	RET

	.INICIO
	.WYZ_PLAYER_ISR
		CALL ROUT
		LD HL,PSG_REG
		LD DE,PSG_REG_SEC
		LD BC,14
		LDIR

		CALL REPRODUCE_SONIDO
		CALL PLAY
		JP REPRODUCE_EFECTO

	; REPRODUCE UN SONIDO POR EL CANAL ESPECIFICADO
	; IN 
	; A = NUMERO DE SONIDO
	; B = CANAL 
	.INICIA_SONIDO 
		LD HL, TABLA_SONIDOS
		CALL EXT_WORD
		LD (PUNTERO_SONIDO), DE
		LD HL, INTERR
		SET 2, (HL)
		LD A, B
		LD HL, TABLA_DATOS_CANAL_SFX
		CALL EXT_WORD
		LD (SONIDO_REGS), DE
	RET

	; REPRODUCE UN FX POR EL CANAL ESPECIFICADO
	; IN 
	; A = NUMERO DE SONIDO
	; B = CANAL 
	.INICIA_EFECTO 
		LD HL, TABLA_EFECTOS
		CALL EXT_WORD
		LD (PUNTERO_EFECTO), DE
		LD HL, INTERR
		SET 3, (HL)
		LD A, B
		LD HL, TABLA_DATOS_CANAL_SFX
		CALL EXT_WORD
		LD (EFECTO_REGS), DE
	RET

	;REPRODUCE EFECTOS DE SONIDO 
	.REPRODUCE_SONIDO
		LD HL, INTERR
		BIT 2,[HL] ;ESTA ACTIVADO EL EFECTO?
		RET Z
		LD HL, (SONIDO_REGS)
		PUSH HL
		POP IX
		LD HL, [PUNTERO_SONIDO]
		CALL REPRODUCE_SONIDO_O_EFECTO
		OR A
		JR Z, REPRODUCE_SONIDO_LIMPIA_BIT
		LD (PUNTERO_SONIDO), HL
		RET
	.REPRODUCE_SONIDO_LIMPIA_BIT
		LD HL, INTERR
		RES 2, (HL)
		RET

	.REPRODUCE_EFECTO
		LD HL, INTERR
		BIT 3, (HL)
		RET Z
		LD HL, (EFECTO_REGS)
		PUSH HL
		POP IX
		LD HL, (PUNTERO_EFECTO)
		CALL REPRODUCE_SONIDO_O_EFECTO
		OR A
		JR Z, REPRODUCE_EFECTO_LIMPIA_BIT
		LD (PUNTERO_EFECTO), HL
		RET
	.REPRODUCE_EFECTO_LIMPIA_BIT
		LD HL, INTERR
		RES 3, (HL)
		RET
		
	.REPRODUCE_SONIDO_O_EFECTO
		LD A,[HL]
		CP $FF
		JR Z,FIN_SONIDO
		LD E, (IX+0) ; IX+0 -> SFX_L LO
		LD D, (IX+1) ; IX+1 -> SFX_L HI
		LD [DE],A
		INC HL
		LD A,[HL]
		RRCA
		RRCA
		RRCA
		RRCA
		AND 00001111B
		LD E, (IX+2) ; IX+2 -> SFX_H LO
		LD D, (IX+3) ; IX+3 -> SFX_H HI
		LD [DE],A
		LD A,[HL]
		AND 00001111B
		LD E, (IX+4) ; IX+4 -> SFX_V LO
		LD D, (IX+5) ; IX+5 -> SFX_V HI
		LD [DE],A
		
		INC HL
		LD A,[HL]
		LD B,A
		BIT 7,A ;09.08.13 BIT MAS SIGINIFICATIVO ACTIVA ENVOLVENTES
		JR Z,NO_ENVOLVENTES_SONIDO
		LD A,$12
		LD [DE],A
		INC HL
		LD A,[HL]
		LD [PSG_REG_SEC+11],A
		INC HL
		LD A,[HL]
		LD [PSG_REG_SEC+12],A
		INC HL
		LD A,[HL]
		CP 1
		JR Z,NO_ENVOLVENTES_SONIDO ;NO ESCRIBE LA ENVOLVENTE SI SU VALOR ES 1
		LD [PSG_REG_SEC+13],A 

	.NO_ENVOLVENTES_SONIDO
		LD A,B
		AND $7F ; RES 7,A ; AND A
		JR Z,NO_RUIDO
		LD [PSG_REG_SEC+6],A
		LD A, (IX+6) ; IX+6 -> SFX_MIX
		JR SI_RUIDO
	.NO_RUIDO XOR A
		LD [PSG_REG_SEC+6],A
		LD A,10111000B
	.SI_RUIDO LD [PSG_REG_SEC+7],A 
		INC HL
		LD A, 1
		RET
	.FIN_SONIDO
		LD A,[ENVOLVENTE_BACK] ;NO RESTAURA LA ENVOLVENTE SI ES 0
		AND A
		JR Z,FIN_NOPLAYER
		;xor a ; ***
		LD [PSG_REG_SEC+13],A ;08.13 RESTAURA LA ENVOLVENTE TRAS EL SFX
		
	.FIN_NOPLAYER
		LD A,10111000B
		LD [PSG_REG_SEC+7],A
		XOR A
		RET 
		
	;VUELCA BUFFER DE SONIDO AL PSG
	.ROUT
		LD A,[PSG_REG+13] 
		AND A ;ES CERO?
		JR Z,NO_BACKUP_ENVOLVENTE
		LD [ENVOLVENTE_BACK],A ;08.13 / GUARDA LA ENVOLVENTE EN EL BACKUP
		XOR A

		.NO_BACKUP_ENVOLVENTE
	;VUELCA BUFFER DE SONIDO AL PSG
		LD HL,PSG_REG_SEC
	.LOUT
		CALL WRITEPSGHL
		INC A
		CP 13
		JR NZ,LOUT
		LD A,[HL]
		AND A
		RET Z
		LD A,13
		CALL WRITEPSGHL
		XOR A
		LD [PSG_REG+13],A
		LD [PSG_REG_SEC+13],A
		RET

	;; A = REGISTER
	;; (HL) = VALUE
	.WRITEPSGHL
		LD B,$F4 
		OUT [C],A 
		LD BC,$F6C0 
		OUT [C],C 
		DEFB $ED 
		DEFB $71 
		LD B,$F5 
		OUTI 
		LD BC,$F680 
		OUT [C],C 
		DEFB $ED 
		DEFB $71 
		RET

	;PLAYER OFF
	.PLAYER_OFF XOR A ;***** IMPORTANTE SI NO HAY MUSICA ****
		LD [INTERR],A
		;LD [FADE],A ;solo si hay fade out

	.CLEAR_PSG_BUFFER
		LD HL,PSG_REG
		LD DE,PSG_REG+1
		LD BC,14
		LD [HL],A
		LDIR
		
		LD A,10111000B ; **** POR SI ACASO ****
		LD [PSG_REG+7],A
		
		LD HL,PSG_REG
		LD DE,PSG_REG_SEC
		LD BC,14
		LDIR 

		JP ROUT

	;CARGA UNA CANCION
	;.IN[A]=N� DE CANCION
	.CARGA_CANCION
		LD HL,INTERR ;CARGA CANCION 
		SET 1,[HL] ;REPRODUCE CANCION
		LD HL,SONG
		LD [HL],A ;N� A

	;DECODIFICAR
	;IN-> INTERR 0 ON
	; SONG
	;CARGA CANCION SI/NO
	.DECODE_SONG LD A,[SONG]

	;LEE CABECERA DE LA CANCION
	;BYTE 0=TEMPO

		LD HL,TABLA_SONG
		CALL EXT_WORD
		LD A,[DE]
		LD [TEMPO],A
		DEC A
		LD [TTEMPO],A
		
	;HEADER BYTE 1
	;[-|-|-|-| 3-1 | 0 ]
	;[-|-|-|-|FX CHN|LOOP]

		INC DE ;LOOP 1=ON/0=OFF?
		LD A,[DE]
		BIT 0,A
		JR Z,NPTJP0
		LD HL,INTERR
		SET 4,[HL]
		
	;SELECCION DEL CANAL DE EFECTOS DE RITMO
	.NPTJP0
		AND 00000110B 
		RRA
		;LD [SELECT_CANAL_P],A

		PUSH DE
		LD HL, TABLA_DATOS_CANAL_SFX
		CALL EXT_WORD
		LD (SONIDO_REGS), DE
		POP HL
		
		INC HL ;2 BYTES RESERVADOS
		INC HL
		INC HL

	;BUSCA Y GUARDA INICIO DE LOS CANALES EN EL MODULO MUS (OPTIMIZAR****************)
	;A�ADE OFFSET DEL LOOP

		PUSH HL ;IX INICIO OFFSETS LOOP POR CANAL
		POP IX
		
		LD DE,$0008 ;HASTA INICIO DEL CANAL A
		ADD HL,DE
		
		
		LD [PUNTERO_P_DECA],HL ;GUARDA PUNTERO INICIO CANAL
		LD E,[IX+0]
		LD D,[IX+1]
		ADD HL,DE
		LD [PUNTERO_L_DECA],HL ;GUARDA PUNTERO INICIO LOOP

		CALL BGICMODBC1
		LD [PUNTERO_P_DECB],HL
		LD E,[IX+2]
		LD D,[IX+3]
		ADD HL,DE
		LD [PUNTERO_L_DECB],HL

		CALL BGICMODBC1
		LD [PUNTERO_P_DECC],HL
		LD E,[IX+4]
		LD D,[IX+5]
		ADD HL,DE
		LD [PUNTERO_L_DECC],HL
		
		CALL BGICMODBC1
		LD [PUNTERO_P_DECP],HL
		LD E,[IX+6]
		LD D,[IX+7]
		ADD HL,DE
		LD [PUNTERO_L_DECP],HL
		
		
	;LEE DATOS DE LAS NOTAS
	;[|][|||||] LONGITUD\NOTA

	.INIT_DECODER
		LD DE,[CANAL_A]
		LD [PUNTERO_A],DE
		LD HL,[PUNTERO_P_DECA]
		CALL DECODE_CANAL ;CANAL A
		LD [PUNTERO_DECA],HL
		
		LD DE,[CANAL_B]
		LD [PUNTERO_B],DE
		LD HL,[PUNTERO_P_DECB]
		CALL DECODE_CANAL ;CANAL B
		LD [PUNTERO_DECB],HL
		
		LD DE,[CANAL_C]
		LD [PUNTERO_C],DE
		LD HL,[PUNTERO_P_DECC]
		CALL DECODE_CANAL ;CANAL C
		LD [PUNTERO_DECC],HL
		
		LD DE,[CANAL_P]
		LD [PUNTERO_P],DE
		LD HL,[PUNTERO_P_DECP]
		CALL DECODE_CANAL ;CANAL P
		LD [PUNTERO_DECP],HL
		
		RET

	;BUSCA INICIO DEL CANAL

	.BGICMODBC1 LD E,$3F ;CODIGO INSTRUMENTO 0
	.BGICMODBC2 XOR A ;BUSCA EL BYTE 0
		LD B,$FF ;EL MODULO DEBE TENER UNA LONGITUD MENOR DE $FF00 ... o_O!
		CPIR
		
		DEC HL
		DEC HL
		LD A,E ;ES EL INSTRUMENTO 0??
		CP [HL]
		INC HL
		INC HL
		JR Z,BGICMODBC2

		DEC HL
		DEC HL
		DEC HL
		LD A,E ;ES VOLUMEN 0??
		CP [HL]
		INC HL
		INC HL
		INC HL
		JR Z,BGICMODBC2
		RET

	;DECODIFICA NOTAS DE UN CANAL
	;IN [DE]=DIRECCION DESTINO
	;NOTA=0 FIN CANAL
	;NOTA=1 SILENCIO
	;NOTA=2 PUNTILLO
	;NOTA=3 COMANDO I

	.DECODE_CANAL
		LD A,[HL]
		AND A ;FIN DEL CANAL?
		JR Z,FIN_DEC_CANAL
		CALL GETLEN

		CP 00000001B ;ES SILENCIO?
		JR NZ,NO_SILENCIO
		OR $40 ; SET 6,A
		JR NO_MODIFICA
		
	.NO_SILENCIO CP 00111110B ;ES PUNTILLO?
		JR NZ,NO_PUNTILLO
		OR A
		RRC B
		XOR A
		JR NO_MODIFICA

	.NO_PUNTILLO CP 00111111B ;ES COMANDO?
		JR NZ,NO_MODIFICA
		BIT 0,B ;COMADO=INSTRUMENTO?
		JR Z,NO_INSTRUMENTO 
		LD A,11000001B ;CODIGO DE INSTRUMENTO 
		LD [DE],A
		INC HL
		INC DE
		LDI ;N� DE INSTRUMENTO
		; LD A,[HL]
		; LD [DE],A
		; INC DE
		; INC HL
		LDI ;VOLUMEN RELATIVO DEL INSTRUMENTO
		; LD A,[HL]
		; LD [DE],A
		; INC DE
		; INC HL
		JR DECODE_CANAL
		
	.NO_INSTRUMENTO BIT 2,B
		JR Z,NO_ENVOLVENTE
		LD A,11000100B ;CODIGO ENVOLVENTE
		LD [DE],A
		INC DE
		INC HL
		LDI
		; LD A,[HL]
		; LD [DE],A
		; INC DE
		; INC HL
		JR DECODE_CANAL
		
	.NO_ENVOLVENTE BIT 1,B
		JR Z,NO_MODIFICA 
		LD A,11000010B ;CODIGO EFECTO
		LD [DE],A 
		INC HL 
		INC DE 
		LD A,[HL] 
		CALL GETLEN 
		
	.NO_MODIFICA LD [DE],A
		INC DE
		XOR A
		DJNZ NO_MODIFICA
		OR $81 ; SET 7,A ; SET 0,A
		LD [DE],A
		INC DE
		INC HL
		RET ;** JR DECODE_CANAL
		
	.FIN_DEC_CANAL OR $80 ; SET 7,A
		LD [DE],A
		INC DE
		RET

	.GETLEN LD B,A
		AND 00111111B
		PUSH AF
		LD A,B
		AND 11000000B
		RLCA
		RLCA
		INC A
		LD B,A
		LD A,10000000B
	.DCBC0 RLCA
		DJNZ DCBC0
		LD B,A
		POP AF
		RET
		
	;PLAY __________________________________________________
	.PLAY LD HL,INTERR ;PLAY BIT 1 ON?
		BIT 1,[HL]
		RET Z
	;TEMPO 
		LD HL,TTEMPO ;CONTADOR TEMPO
		INC [HL]
		LD A,[TEMPO]
		CP [HL]
		JR NZ,PAUTAS
		LD [HL],0
		
	;INTERPRETA 
		LD IY,PSG_REG
		LD IX,PUNTERO_A
		LD BC,PSG_REG+8
		CALL LOCALIZA_NOTA
		LD IY,PSG_REG+2
		LD IX,PUNTERO_B
		LD BC,PSG_REG+9
		CALL LOCALIZA_NOTA
		LD IY,PSG_REG+4
		LD IX,PUNTERO_C
		LD BC,PSG_REG+10
		CALL LOCALIZA_NOTA
		LD IX,PUNTERO_P ;EL CANAL DE EFECTOS ENMASCARA OTRO CANAL
		CALL LOCALIZA_EFECTO 

	;PAUTAS 
		
	.PAUTAS 
		LD IY,PSG_REG+0
		LD IX,PUNTERO_P_A
		LD HL,PSG_REG+8
		CALL PAUTA ;PAUTA CANAL A
		LD IY,PSG_REG+2
		LD IX,PUNTERO_P_B
		LD HL,PSG_REG+9
		CALL PAUTA ;PAUTA CANAL B
		LD IY,PSG_REG+4
		LD IX,PUNTERO_P_C
		LD HL,PSG_REG+10
		JP PAUTA ;PAUTA CANAL C 
		

		


	;LOCALIZA NOTA CANAL A
	;IN [PUNTERO_A]

	;LOCALIZA NOTA CANAL A
	;IN [PUNTERO_A]

	.LOCALIZA_NOTA 
		LD L,[IX+PUNTERO_A-PUNTERO_A] ;HL=[PUNTERO_A_C_B]
		LD H,[IX+PUNTERO_A-PUNTERO_A+1]
		LD A,[HL]
		AND 11000000B ;COMANDO?
		CP 11000000B
		JR NZ,LNJP0

	;BIT[0]=INSTRUMENTO
		
	.COMANDOS 
		LD A,[HL]
		BIT 0,A ;INSTRUMENTO
		JR Z,COM_EFECTO

		INC HL
		LD A,[HL] ;N� DE PAUTA
		INC HL
		LD E,[HL] 
		
		PUSH HL ;;TEMPO ******************
		LD HL,TEMPO
		BIT 5,E
		JR Z,NO_DEC_TEMPO
		DEC [HL]
	.NO_DEC_TEMPO 
		BIT 6,E
		JR Z,NO_INC_TEMPO
		INC [HL]
	.NO_INC_TEMPO 
		RES 5,E ;SIEMPRE RESETEA LOS BITS DE TEMPO 
		RES 6,E
		POP HL
		
		LD [IX+VOL_INST_A-PUNTERO_A],E ;REGISTRO DEL VOLUMEN RELATIVO
		INC HL
		LD [IX+PUNTERO_A-PUNTERO_A],L
		LD [IX+PUNTERO_A-PUNTERO_A+1],H
		LD HL,TABLA_PAUTAS
		CALL EXT_WORD
		LD [IX+PUNTERO_P_A0-PUNTERO_A],E
		LD [IX+PUNTERO_P_A0-PUNTERO_A+1],D
		LD [IX+PUNTERO_P_A-PUNTERO_A],E
		LD [IX+PUNTERO_P_A-PUNTERO_A+1],D
		LD L,C
		LD H,B
		RES 4,[HL] ;APAGA EFECTO ENVOLVENTE
		XOR A
		LD [PSG_REG_SEC+13],A
		LD [PSG_REG+13],A
		;LD [ENVOLVENTE_BACK],A ;08.13 / RESETEA EL BACKUP DE LA ENVOLVENTE
		JR LOCALIZA_NOTA

	.COM_EFECTO 
		BIT 1,A ;EFECTO DE SONIDO
		JR Z,COM_ENVOLVENTE

		INC HL
		LD A,[HL]
		INC HL
		LD [IX+PUNTERO_A-PUNTERO_A],L
		LD [IX+PUNTERO_A-PUNTERO_A+1],H
		JP INICIA_SONIDO

	.COM_ENVOLVENTE 
		BIT 2,A
		RET Z ;IGNORA - ERROR 
		
		INC HL
		LD A,[HL] ;CARGA CODIGO DE ENVOLVENTE
		LD [ENVOLVENTE],A
		INC HL
		LD [IX+PUNTERO_A-PUNTERO_A],L
		LD [IX+PUNTERO_A-PUNTERO_A+1],H
		LD L,C
		LD H,B
		LD [HL],00010000B ;ENCIENDE EFECTO ENVOLVENTE
		JR LOCALIZA_NOTA
		
		
	.LNJP0 
		LD A,[HL]
		INC HL
		BIT 7,A
		JR Z,NO_FIN_CANAL_A ;
		BIT 0,A
		JR Z,FIN_CANAL_A

	.FIN_NOTA_A 
		LD E,[IX+CANAL_A-PUNTERO_A]
		LD D,[IX+CANAL_A-PUNTERO_A+1] ;PUNTERO BUFFER AL INICIO
		LD [IX+PUNTERO_A-PUNTERO_A],E
		LD [IX+PUNTERO_A-PUNTERO_A+1],D
		LD L,[IX+PUNTERO_DECA-PUNTERO_A] ;CARGA PUNTERO DECODER
		LD H,[IX+PUNTERO_DECA-PUNTERO_A+1]
		PUSH BC
		CALL DECODE_CANAL ;DECODIFICA CANAL
		POP BC
		LD [IX+PUNTERO_DECA-PUNTERO_A],L ;GUARDA PUNTERO DECODER
		LD [IX+PUNTERO_DECA-PUNTERO_A+1],H
		JP LOCALIZA_NOTA
		
	.FIN_CANAL_A 
		LD HL,INTERR ;LOOP?
		BIT 4,[HL] 
		JR NZ,FCA_CONT
		POP AF
		JP PLAYER_OFF
		

	.FCA_CONT 
		LD L,[IX+PUNTERO_L_DECA-PUNTERO_A] ;CARGA PUNTERO INICIAL DECODER
		LD H,[IX+PUNTERO_L_DECA-PUNTERO_A+1]
		LD [IX+PUNTERO_DECA-PUNTERO_A],L
		LD [IX+PUNTERO_DECA-PUNTERO_A+1],H
		JR FIN_NOTA_A
		
	.NO_FIN_CANAL_A 
		LD [IX+PUNTERO_A-PUNTERO_A],L ;[PUNTERO_A_B_C]=HL GUARDA PUNTERO
		LD [IX+PUNTERO_A-PUNTERO_A+1],H
		AND A ;NO REPRODUCE NOTA SI NOTA=0
		JR Z,FIN_RUTINA
		BIT 6,A ;SILENCIO?
		JR Z,NO_SILENCIO_A
		LD A,[BC]
		AND 00010000B
		JR NZ,SILENCIO_ENVOLVENTE
		
		XOR A
		LD [BC],A ;RESET VOLUMEN DEL CORRESPODIENTE CHIP
		LD [IY+0],A
		LD [IY+1],A
		RET
		
	.SILENCIO_ENVOLVENTE
		LD A,$FF
		LD [PSG_REG+11],A
		LD [PSG_REG+12],A 
		XOR A
		LD [PSG_REG+13],A 
		LD [IY+0],A
		LD [IY+1],A
		RET

	.NO_SILENCIO_A 
		LD [IX+REG_NOTA_A-PUNTERO_A],A ;REGISTRO DE LA NOTA DEL CANAL 
		CALL NOTA ;REPRODUCE NOTA
		LD L,[IX+PUNTERO_P_A0-PUNTERO_A] ;HL=[PUNTERO_P_A0] RESETEA PAUTA 
		LD H,[IX+PUNTERO_P_A0-PUNTERO_A+1]
		LD [IX+PUNTERO_P_A-PUNTERO_A],L ;[PUNTERO_P_A]=HL
		LD [IX+PUNTERO_P_A-PUNTERO_A+1],H
	.FIN_RUTINA 
		RET


	;LOCALIZA EFECTO
	;IN HL=[PUNTERO_P]

	.LOCALIZA_EFECTOLD L,[IX+0] ;HL=[PUNTERO_P]
		LD H,[IX+1]
		LD A,[HL]
		CP 11000010B
		JR NZ,LEJP0

		INC HL
		LD A,[HL]
		INC HL
		LD [IX+00],L
		LD [IX+01],H
		JP INICIA_SONIDO
		
		
	.LEJP0 
		INC HL
		BIT 7,A
		JR Z,NO_FIN_CANAL_P ;
		BIT 0,A
		JR Z,FIN_CANAL_P
	.FIN_NOTA_P 
		LD DE,[CANAL_P]
		LD [IX+0],E
		LD [IX+1],D
		LD HL,[PUNTERO_DECP] ;CARGA PUNTERO DECODER
		PUSH BC
		CALL DECODE_CANAL ;DECODIFICA CANAL
		POP BC
		LD [PUNTERO_DECP],HL ;GUARDA PUNTERO DECODER
		JP LOCALIZA_EFECTO
		
	.FIN_CANAL_P 
		LD HL,[PUNTERO_L_DECP] ;CARGA PUNTERO INICIAL DECODER
		LD [PUNTERO_DECP],HL
		JR FIN_NOTA_P
		
	.NO_FIN_CANAL_P 
		LD [IX+0],L ;[PUNTERO_A_B_C]=HL GUARDA PUNTERO
		LD [IX+1],H
		RET

	; PAUTA DE LOS 3 CANALES
	; .IN[IX]:PUNTERO DE LA PAUTA
	; [HL]:REGISTRO DE VOLUMEN
	; [IY]:REGISTROS DE FRECUENCIA

	; FORMATO PAUTA 
	; 7 6 5 4 3-0 3-0 
	; BYTE 1 [LOOP|OCT-1|OCT+1|ORNMT|VOL] - BYTE 2 [ | | | |PITCH/NOTA]

	.PAUTA 
		BIT 4,[HL] ;SI LA ENVOLVENTE ESTA ACTIVADA NO ACTUA PAUTA
		RET NZ

		LD A,[IY+0]
		LD B,[IY+1]
		OR B
		RET Z


		PUSH HL
		
	.PCAJP4 
		LD L,[IX+0]
		LD H,[IX+1] 
		LD A,[HL]
		
		BIT 7,A ;LOOP / EL RESTO DE BITS NO AFECTAN
		JR Z,PCAJP0
		AND 00011111B ;M�XIMO LOOP PAUTA [0,32]X2!!!-> PARA ORNAMENTOS
		RLCA ;X2
		LD D,0
		LD E,A
		SBC HL,DE
		LD A,[HL]

	.PCAJP0 
		BIT 6,A ;OCTAVA -1
		JR Z,PCAJP1
		LD E,[IY+0]
		LD D,[IY+1]

		AND A
		RRC D
		RR E
		LD [IY+0],E
		LD [IY+1],D
		JR PCAJP2
		
	.PCAJP1 
		BIT 5,A ;OCTAVA +1
		JR Z,PCAJP2
		LD E,[IY+0]
		LD D,[IY+1]

		AND A
		RLC E
		RL D
		LD [IY+0],E
		LD [IY+1],D 




	.PCAJP2 
		LD A,[HL]
		BIT 4,A
		JR NZ,PCAJP6 ;ORNAMENTOS SELECCIONADOS

		INC HL ;______________________ FUNCION PITCH DE FRECUENCIA__________________ 
		PUSH HL
		LD E,A
		LD A,[HL] ;PITCH DE FRECUENCIA
		LD L,A
		AND A
		LD A,E
		JR Z,ORNMJP1

		LD A,[IY+0] ;SI LA FRECUENCIA ES 0 NO HAY PITCH
		ADD A,[IY+1]
		AND A
		LD A,E
		JR Z,ORNMJP1
		

		BIT 7,L
		JR Z,ORNNEG
		LD H,$FF
		JR PCAJP3
	.ORNNEG 
		LD H,0
		
	.PCAJP3 
		LD E,[IY+0]
		LD D,[IY+1]
		ADC HL,DE
		LD [IY+0],L
		LD [IY+1],H
		JR ORNMJP1


	.PCAJP6 
		INC HL ;______________________ FUNCION ORNAMENTOS__________________ 
		
		PUSH HL
		PUSH AF
		LD A,[IX+REG_NOTA_A-PUNTERO_P_A] ;RECUPERA REGISTRO DE NOTA EN EL CANAL
		LD E,[HL] ;
		ADC E ;+- NOTA 
		CALL TABLA_NOTAS
		POP AF 
		
		
	.ORNMJP1 
		POP HL
		
		INC HL
		LD [IX+0],L
		LD [IX+1],H
	.PCAJP5 
		POP HL
		LD B,[IX+VOL_INST_A-PUNTERO_P_A] ;VOLUMEN RELATIVO
		ADD B
		JP P,PCAJP7
		LD A,1 ;NO SE EXTIGUE EL VOLUMEN
	.PCAJP7 
		AND 00001111B ;VOLUMEN FINAL MODULADO
		LD [HL],A
		RET



	;NOTA -  REPRODUCE UNA NOTA
	;IN [A]=CODIGO DE LA NOTA
	; [IY]=REGISTROS DE FRECUENCIA


	.NOTA 
		LD L,C
		LD H,B
		BIT 4,[HL]
		LD B,A
		JR NZ,EVOLVENTES
		LD A,B
	.TABLA_NOTAS 
		LD HL,DATOS_NOTAS ;BUSCA FRECUENCIA
		CALL EXT_WORD
		LD [IY+0],E
		LD [IY+1],D
		RET




	;IN [A]=CODIGO DE LA ENVOLVENTE
	; [IY]=REGISTRO DE FRECUENCIA

	.EVOLVENTES 
		LD HL,DATOS_NOTAS
		;SUB 12
		RLCA ;X2
		LD D,0
		LD E,A
		ADD HL,DE
		LD E,[HL]
		INC HL
		LD D,[HL]
		
		PUSH DE
		LD A,[ENVOLVENTE] ;FRECUENCIA DEL CANAL ON/OFF
		RRA
		JR NC,FRECUENCIA_OFF
		LD [IY+0],E
		LD [IY+1],D
		JR CONT_ENV
		
	.FRECUENCIA_OFF 
		LD DE,$0000
		LD [IY+0],E
		LD [IY+1],D
		;CALCULO DEL RATIO (OCTAVA ARRIBA)
	.CONT_ENV 
		POP DE
		PUSH AF
		PUSH BC
		AND 00000011B
		LD B,A
		;INC B
		
		;AND A ;1/2
		RR D
		RR E
	.CRTBC0 
		;AND A ;1/4 - 1/8 - 1/16
		RR D
		RR E
		DJNZ CRTBC0
		LD A,E
		LD [PSG_REG+11],A
		LD A,D
		AND 00000011B
		LD [PSG_REG+12],A
		POP BC
		POP AF ;SELECCION FORMA DE ENVOLVENTE
		
		RRA
		AND 00000110B ;$08,$0A,$0C,$0E
		ADD 8 
		LD [PSG_REG+13],A
		LD [ENVOLVENTE_BACK],A
		RET


	;EXTRAE UN WORD DE UNA TABLA
	;.IN[HL]=DIRECCION TABLA
	; [A]= POSICION
	;OUT[DE]=WORD

	.EXT_WORD
		LD D,0
		RLCA
		LD E,A
		ADD HL,DE
		LD E,[HL]
		INC HL
		LD D,[HL]
		RET
		
	;TABLA DE DATOS DEL SELECTOR DEL CANAL DE EFECTOS DE RITMO

	.TABLA_DATOS_CANAL_SFX
		defw SELECT_CANAL_A,SELECT_CANAL_B,SELECT_CANAL_C


	;BYTE 0:SFX_L 
	;BYTE 1:SFX_H 
	;BYTE 2:SFX_V 
	;BYTE 3:SFX_MIX

	.SELECT_CANAL_A 
		defw PSG_REG_SEC+0, PSG_REG_SEC+1, PSG_REG_SEC+8
		defb 10110001B
		
	.SELECT_CANAL_B 
		defw PSG_REG_SEC+2, PSG_REG_SEC+3, PSG_REG_SEC+9
		defb 10101010B
		
	.SELECT_CANAL_C 
		defw PSG_REG_SEC+4, PSG_REG_SEC+5, PSG_REG_SEC+10
		defb 10011100B


	;____________________________________


	.INTERR 
		defb 0	;INTERRUPTORES 1=ON 0=OFF
				;BIT 0=CARGA CANCION ON/OFF
				;BIT 1=PLAYER ON/OFF
				;BIT 2=EFECTOS ON/OFF
				;BIT 3=SFX ON/OFF
				;BIT 4=LOOP
	
	;MUSICA **** EL ORDEN DE LAS VARIABLES ES FIJO ******

	.SONG 			defb 0	;DBN� DE CANCION
	.TEMPO 			defb 0	;DB TEMPO
	.TTEMPO 		defb 0	;DB CONTADOR TEMPO

	.PUNTERO_A 		defw 0	;DW PUNTERO DEL CANAL A
	.PUNTERO_B 		defw 0	;DW PUNTERO DEL CANAL B
	.PUNTERO_C 		defw 0	;DW PUNTERO DEL CANAL C

	.CANAL_A 		defw 0	;DW DIRECION DE INICIO DE LA MUSICA A
	.CANAL_B 		defw 0	;DW DIRECION DE INICIO DE LA MUSICA B
	.CANAL_C 		defw 0	;DW DIRECION DE INICIO DE LA MUSICA C

	.PUNTERO_P_A 	defw 0	;DW PUNTERO PAUTA CANAL A
	.PUNTERO_P_B 	defw 0	;DW PUNTERO PAUTA CANAL B
	.PUNTERO_P_C 	defw 0	;DW PUNTERO PAUTA CANAL C

	.PUNTERO_P_A0 	defw 0	;DW INI PUNTERO PAUTA CANAL A
	.PUNTERO_P_B0 	defw 0	;DW INI PUNTERO PAUTA CANAL B
	.PUNTERO_P_C0 	defw 0	;DW INI PUNTERO PAUTA CANAL C

	.PUNTERO_P_DECA defw 0	;DW PUNTERO DE INICIO DEL DECODER CANAL A
	.PUNTERO_P_DECB defw 0	;DW PUNTERO DE INICIO DEL DECODER CANAL B
	.PUNTERO_P_DECC defw 0	;DW PUNTERO DE INICIO DEL DECODER CANAL C

	.PUNTERO_DECA 	defw 0	;DW PUNTERO DECODER CANAL A
	.PUNTERO_DECB 	defw 0	;DW PUNTERO DECODER CANAL B
	.PUNTERO_DECC 	defw 0	;DW PUNTERO DECODER CANAL C 

	.REG_NOTA_A 	defb 0	;DB REGISTRO DE LA NOTA EN EL CANAL A
	.VOL_INST_A 	defb 0	;DB VOLUMEN RELATIVO DEL INSTRUMENTO DEL CANAL A
	.REG_NOTA_B 	defb 0	;DB REGISTRO DE LA NOTA EN EL CANAL B
	.VOL_INST_B 	defb 0	;DB VOLUMEN RELATIVO DEL INSTRUMENTO DEL CANAL B ;VACIO
	.REG_NOTA_C 	defb 0	;DB REGISTRO DE LA NOTA EN EL CANAL C
	.VOL_INST_C 	defb 0	;DB VOLUMEN RELATIVO DEL INSTRUMENTO DEL CANAL C

	.PUNTERO_L_DECA defw 0	;DW PUNTERO DE INICIO DEL LOOP DEL DECODER CANAL A
	.PUNTERO_L_DECB defw 0	;DW PUNTERO DE INICIO DEL LOOP DEL DECODER CANAL B
	.PUNTERO_L_DECC defw 0	;DW PUNTERO DE INICIO DEL LOOP DEL DECODER CANAL C

	;CANAL DE EFECTOS DE RITMO - ENMASCARA OTRO CANAL

	.PUNTERO_P 		defw 0	;DW PUNTERO DEL CANAL EFECTOS
	.CANAL_P 		defw 0	;DW DIRECION DE INICIO DE LOS EFECTOS
	.PUNTERO_P_DECP defw 0	;DW PUNTERO DE INICIO DEL DECODER CANAL P
	.PUNTERO_DECP 	defw 0	;DW PUNTERO DECODER CANAL P
	.PUNTERO_L_DECP defw 0	;DW PUNTERO DE INICIO DEL LOOP DEL DECODER CANAL P
	;SELECT_CANAL_P defb INTERR+$36 ;DB SELECCION DE CANAL DE EFECTOS DE RITMO

	.SFX_L 			defw 0	;DW DIRECCION BUFFER EFECTOS DE RITMO REGISTRO BAJO
	.SFX_H 			defw 0	;DW DIRECCION BUFFER EFECTOS DE RITMO REGISTRO ALTO
	.SFX_V 			defw 0	;DW DIRECCION BUFFER EFECTOS DE RITMO REGISTRO VOLUMEN
	.SFX_MIX 		defw 0	;DW DIRECCION BUFFER EFECTOS DE RITMO REGISTRO MIXER

	;EFECTOS DE SONIDO

	.N_SONIDO 		defb 0	;DB : NUMERO DE SONIDO
	.PUNTERO_SONIDO defw 0	;DW : PUNTERO DEL SONIDO QUE SE REPRODUCE

	;DB [13] BUFFERs DE REGISTROS DEL PSG

	.PSG_REG 		defs 16
	.PSG_REG_SEC 	defs 16
	.ENVOLVENTE 	
		defb 0  ;DB : FORMA DE LA ENVOLVENTE
				;BIT 0 : FRECUENCIA CANAL ON/OFF
				;BIT 1-2 : RATIO 
				;BIT 3-3 : FORMA 
	.ENVOLVENTE_BACK defb 0 ;.defb BACKUP DE LA FORMA DE LA ENVOLENTE
#endasm

void wyz_init (void) {
	#asm
		call WYZPLAYER_INIT
	#endasm	
}

void __FASTCALL__ wyz_play_music (unsigned char m) {
	#asm
		; Song number is in L
		ld  a, l
		call CARGA_CANCION
	#endasm
}

void __FASTCALL__ wyz_play_sound (unsigned char s) {
	#asm
		; Sound number is in L
		ld  a, l
		ld  b, WYZ_FX_CHANNEL
		call INICIA_EFECTO
	#endasm
}

void wyz_stop_sound (void) {
	#asm
		call PLAYER_OFF
	#endasm
}

