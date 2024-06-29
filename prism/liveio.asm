	TITLE	LIVEIO Module. 21-June-1989
	subttl	Basic constants
	page
;********************************************	
;
;
bufsiz	equ	16384
mntrgh	equ	bufsiz * 3/4

true	equ	1
false	equ	0

mportd	equ	03F8H		; address of port data
mporti	equ	03F9H		; Int enable register
mportri equ	03FAH		; read int id.
mportc	equ	03FBH		; line control.
mportm	equ	03FCH		; modem control.
mports	equ	03FDH		; address of line status
mportms equ	03FEH		; modem status.

ocw1	equ	0021H		; 8259 OCW1 address.
intvec	equ	0030H		; address of interrupt vector
eoicom	equ	0064H		; end of interrupt 4
inten	equ	00EFH		; mask to enable int 4 (com1)

cbaud	equ	0CH		; default speed setting (9600)

ibeats	equ	6215		; 96 ticks/beat 120 beats/min

	subttl	Macros

push_m	macro arglist
		irp	register,<arglist>
		push	register
		endm
	endm

pop_m	macro arglist
		irp	register,<arglist>
		pop	register
		endm
	endm

push_all macro
		push_m <ax,bx,cx,dx,di,si,es,ds,bp>
	 endm

pop_all  macro
		pop_m <bp,ds,es,si,di,dx,cx,bx,ax>
	 endm

pushrg	macro
		push_m <di,si,bp,ds,es>
	endm

poprg	macro
		pop_m <es,ds,bp,si,di>
	endm

	subttl	Data segment defs
	page
;************************************
;
;
DGROUP	GROUP	DATA
DATA	SEGMENT	WORD PUBLIC 'DATA'
	ASSUME	DS:DGROUP
	public	spd,sch,alltx,goodtx


	source	db	bufsiz DUP(?)	; buffer for data from port
	bufptr	dw	0	; pointer in buffer
	count	dw	0	; number of chars in buffer
	firiv	dw	?	; save area for interrupt vector
	fircs	dw	?	; save area for code segement of IV
	savesi	dw	0	; save area for si reg
	portin	db	0	; has port been initialised
	xofsnt	db	0
	xofrcv	db	0
	spd	dw	0	; port speed 
	sch	db	0	; passed char to send to port
; more tx stuff
	txfull	dw	0	; tx in progress.	

; from tsr
	tcurcnt dw	0	; current fraction.
	tcurval dw	0	; cur timer value.
	setip 	dw	0	; saved ip for int 82h
	setseg  dw	0

	mclk	dw	0
	mtick	dw	0

	alltx	dw	0
	goodtx	dw	0
	
DATA	ENDS

	subttl	Code segment
	page
;**********************************
;
;
PGROUP	GROUP	PROG
PROG	SEGMENT	BYTE PUBLIC 'PROG'
	ASSUME 	CS:PGROUP
	
	PUBLIC	serini, setspd, serput, serget, serrst, clrbuf, chaval
	public	setbeat, gettick


	SUBTTL	Serini
;		
; function serini - initialises serial port
;		    function call: serini()
;
SERINI	PROC	FAR
	pushrg
	cmp	portin,0	; has port been initialised
	je	serin1
	poprg
	ret

serin1:	cli
	cld
;
	push 	es
	xor 	ax,ax
	mov	es,ax			; address low memory
	mov	bx,intvec		; get interrupt vector address
	mov	ax,es:[bx]		; get current interrupt vector
	mov	firiv,ax		; save current interrupt vector
	mov	ax,offset serint	; get pointer to new routine
	mov	es:[bx],ax		; and set table to point to it
	mov	ax,es:[bx+2]
	mov	fircs,ax		; and save it as well
	mov	es:[bx+2],cs
	mov	portin,1		; remember port initialised
	mov	ax,offset source	; clear input buffer
	mov	bufptr,ax
	mov	savesi,ax
	mov	count,0

; timer int 8 (20h)
	mov	bx,20h
	mov	ax,es:[bx]
	mov	word ptr cs:tvip,ax
	mov	ax,es:[bx+2]
	mov	word ptr cs:tvseg,ax
	mov	word ptr es:[bx],offset timint		; new timer int routine
	mov	word ptr es:[bx+2],cs

	pop	es
;
;
	in	al,ocw1		; set up 8259 interrupt controller OCW1 mask.
	and	al,inten
	out	ocw1,al			; enable INT4
;
; setup uart.
	mov	dx,mportc
	mov	al,3		; 8 data bits.
	out	dx,al		; set up serial card line control reg
	mov	dx,mportd
	in	al,dx		; flush UART's receive buffer
	mov	dx,mporti
	mov	al,3		; RX (0) and TX (1)
	out	dx,al		; set up interrupt enable reg
	mov	dx,mportm	; modem control.
	mov	al,0FH		; dtr,rts,out1,out2
	out	dx,al
	mov	dx,mportri
	in	al,dx		; clear any pending ints.

;
;
	mov	dx,mportc	; initialise port speed
	in	al,dx
	mov	bl,al
	or	ax,80H
	out	dx,al
	mov	dx,mportd
	mov	ax,cbaud
	out	dx,al		; low byte
	inc	dx
	mov	al,ah
	out	dx,al		; high byte.
	mov	dx,mportc
	and	al,07fh		; clear bit.
	mov	al,bl
	out	dx,al
;
; code from tsr program.
;

; setup a new timer 0 value.
	mov dx,ibeats
	mov tcurval,dx
	mov mclk,0		; init midi microbeat counter
	mov mtick,0
	mov al,36h
	out 43h,al		; timer 0 mode word.
	mov al,dl
	out 40h,al
	mov al,dh
	out 40h,al

	in al,61h
	and al,0fch
	out 61h,al

	sti			; initialisation finished
	poprg
	ret
SERINI	ENDP



	subttl	serint - serial interrupt
	page
; function - serint : interrupt vector that is patched by serini
;
;
SERINT	PROC	FAR
	push	ax
	push	dx
	mov	dx,mportri	; int id register.
	in	al,dx		; get it.
	mov	ah,al		; save.
	and	al,1		; int pending?
	jz	serint1
	pop	dx
	jmp	short retint1	; no int pending!
serint1:and	ah,7		; mask.
	cmp	ah,4		; rxint?
	jnz	serint2		; no.
	pop	dx
	pop	ax
	jmp	short serrec
serint2:cmp	ah,2		; txint?
	jnz	serint3
	pop	dx
	pop	ax
	jmp	sertbe		; tx buffer empty.
serint3:pop	dx
	jmp	short retint1	; some other. (error)
;
sertbe: push	ax
	push	dx
	push	ds
	mov	ax,seg data
	mov	ds,ax
	inc	alltx
	mov	dx,mports
	in	al,dx
	and	al,020H
	jz	sertbef
	mov	txfull,0	; flag tx not in progress.
	inc	goodtx
sertbef:pop	ds
	pop	dx
	jmp	short retint1

; serial input.
serrec: push	ax
	push	ds
	push	bx
	push	dx
	push	es
	push	di
	push	bp
	push	cx			; save regs to make transparant
	cld
	mov	ax,seg data
	mov	ds,ax				; address data segment
	mov	es,ax
	mov	di,bufptr			; di equals buffer pointer
	mov	dx,mports
	in	al,dx
	test	al,1			; data available
	jz	retint
	mov	dx,mportd
	in	al,dx			; get data
srint1:	stosb
	cmp	di,offset source + bufsiz	; past end of buffer
	jb	srint2
	mov	di,offset source	; wrap round
srint2:	inc	count
retint:	mov	bufptr,di		; store pointer
	pop	cx
	pop	bp
	pop	di
	pop	es
	pop	dx
	pop	bx
	pop	ds
retint1:mov	al,eoicom
	out	20H,al			; send end of interrupt
	pop	ax
	iret
SERINT	ENDP


	subttl	serrst - Tidy up at end.
	page
; function - serrst : resets interrupt vector
;		      function call: serrst() 
;
SERRST	PROC	FAR
	pushrg
	mov	ax,seg data
	mov	ds,ax

	cmp	portin,0	; has it been set ?
	je	srst1
	cli
; reset the timer speed.
	mov al,36h
	out 43h,al		; load timer 0 mode word
	mov al,0h
	out 40h,al
	mov al,0h
	out 40h,al		; set value to max.
;
	sti
	xor	ax,ax
serrstl:dec	ax
	jnz	serrstl		; delay.
;
	cli
	mov	dx,mportm	; model control register
	mov	al,0FH
	out	dx,al

	mov	dx,mporti
	xor	al,al
	out	dx,al		; disable ints in chip.

	mov 	dx,mportri
	in	al,dx		; clear ints.

	in	al,ocw1
	or	al,10H		; disable int 4	
	out	ocw1,al

	xor	bx,bx
	push	es
	mov	es,bx
	mov	bx,intvec		; pick up initial values
	mov	ax,firiv
	mov	es:[bx],ax
	mov	ax,fircs
	mov	es:[bx+2],ax
	mov	dx,mportm		; reset those set by serini
	mov	al,0FH
	out	dx,al
	mov	dx,mportms
	in	al,dx

	mov	portin,0		; now port reset
; restore timer vector.
	
	mov bx,word ptr cs:tvip
	mov word ptr es:[20h],bx
	mov bx,word ptr cs:tvseg
	mov word ptr es:[22h],bx

	pop	es
; 

	sti
srst1:	poprg
	ret
SERRST	ENDP



	subttl	setspd - set port speed
	page
; function setspd : Sets port speed. Passed variable, if value
;
; takes value 9600 baud for any other value
; 
; function call: setspd(num)
; 		
SETSPD	PROC	FAR
	push	bp
	mov	bp,sp
	mov	ax,[bp+6]
	xor	ah,ah
;	mov	ax,0CH		; preset speed to 9600.
	mov	spd,ax
	pushrg
	mov	dx,mportc
	in	al,dx
	mov	bl,al
	or	ax,80H
	out	dx,al
	mov	dx,mportd
	mov	ax,spd
setsp6:	out	dx,al
	inc	dx
	mov	al,ah
	out	dx,al
	mov	dx,mportc
	mov	al,bl
	out	dx,al
	poprg
	pop	bp
	ret
SETSPD	ENDP



; function - clrbuf : clears input buffer
;		      function call: clrbuf()
;
CLRBUF	PROC	FAR
	cli
	push	ds
	mov	ax,seg data
	mov	ds,ax
	mov	ax,offset source
	mov	bufptr,ax
	mov	savesi,ax
	mov	count,0
	pop	ds
	sti
	ret
CLRBUF	ENDP


; function - serput : sends character to serial port
;		      expects integer argument
;		      returns 1 or 0 indicating success/failure	
;		      function call: succ = serput(char_to_send)
SERPUT	PROC	FAR
	push	bp			; pick up char from args
	mov	bp,sp
	pushrg
;
	mov	ax,seg data
	mov	ds,ax
	mov	ax,[bp+6]		; get the character.
	xor	ah,ah
	mov	sch,al
;
sendc3:	push	dx
;	mov	dx,mports
;	in	al,dx
;	and	al,20h
;	jz	sendc7		; who kidding, tx full.
	sub	cx,cx			; set number of tries
sendc4: cmp	txfull,0	; tx in progress?
	jz	sendc5		; no.
	loop	sendc4
	jmp	sendc6		; problem port not ready
sendc5:	mov	dx,mportd
	mov	txfull,1	; set tx in progress.
	mov	al,sch		; get the character.
	out	dx,al
	pop	dx
	poprg
	pop	bp
	mov	ax,1		; all ok
	ret

sendc6:	pop	dx
	poprg	
	pop	bp 
	mov	ax,0		; not so good	
	ret

sendc7:	pop	dx
	poprg	
	pop	bp 
	mov	ax,2		; not so good	
	ret
SERPUT	ENDP



; function SERGET : gets characters from the serial port. 
;		    char returned in AX
;		    returns <0 if no char	
;		    function call: intval = serget()
SERGET	PROC	FAR
	pushrg
	cmp	count,0			; any chars available ?
	je getch9
	mov	si,savesi
	lodsb
	xor	ah,ah
	cmp	si,offset source + bufsiz
	jb	getch7
	mov	si,offset source	; wrap round
getch7:	dec count
	mov	savesi,si
getch8:	poprg
	ret

getch9:	mov	ax,0ff00h
	poprg
	ret
SERGET	ENDP


; function chaval : returns number of characters in buffer

CHAVAL	PROC	FAR
	mov	ax,count		; return number of chars in buffer
	ret
CHAVAL	ENDP		


	subttl	Gettick
;;; gettick
; returns tick count in ax
;
gettick	proc	far
	push	ds
	mov	ax,seg data
	mov	ds,ax
	mov	ax,mtick
	pop	ds
	ret
gettick endp


	subttl Setbeat
;;; setbeat( beats )
; set the beat rate.
;
setbeat	proc	far
	push	bp
	mov	bp,sp
	push	ds
	mov	ax,seg data
	mov	ds,ax

	mov	ax,[bp+6]
	cli
	mov	tcurval,ax
	push	ax
	mov	al,36h
	out	43h,al
	pop	ax
	out	40h,al
	mov	al,ah
	out	40h,al
	sti
	mov	mclk,0		; restart metronome.

	pop	ds
	pop	bp
	ret
setbeat endp

	subttl	Timer int routine
	page
; timint - timer interrupt.
;
timint:	push ax
	push ds
	mov ax,seg data
	mov ds,ax

	inc mtick
	inc mclk
	cmp mclk,96
	jne timint1		; not a beat.
; blip speaker
;	in al,61h
;	xor al,2
;	out 61h,al
;
	mov mclk,0

timint1:mov ax,tcurval
	and ax,ax
	jz timchain		; jmp to the old routine.
	add tcurcnt,ax
	jc timchain		; counter has rolled over.
	pop ds
	mov al,20h		; EOI.
	out 20h,al
	pop ax
	iret

timchain:pop ds
	pop ax
	jmp tintxeq		; jump to old routine.


; timer variables. Part of code!
;
tintxeq:db 0eah			; jump far op code.
tvip:	dw 0
tvseg:	dw 0

PROG	ENDS
	END
;--------------------------------------------------------------
