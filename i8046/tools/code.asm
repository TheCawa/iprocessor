.org 0x000000

.text
main:
  LDI.A    SCS, 0x00FFFF  ; Инициализация SCS
  CLI              
  LDI.A     SSS, 0x000400  ; Делаем стек
  LDI.A      DS, 0x020000  ; Сегмент данных монтируем в начале области рабочей памяти
  LDI.A      SS, 0x025555  ; Пушим стек где попало
  LDI.A     ES, 0x02AAAA  ; хз, просто
  LDI.A     SDS, 0x000400  ; Выделяем 1024 байт под сегмент данных
    
  LDI.A     IY, 0x010001  ; Ща будем работать с областью видеопамяти
  LDI.A      IX, message
  MOV      A0, IX
  ADD.A      A0, 0x00000C
    
  LDI.B     XL1, 0x0F
  CALL    output
  HALT
    
output:
  LOD.B    XH1, [IX]
  STR.W      X1, [IY]
  INC      IX
  ADD.A      IY, 0x000002
  CMP       IX, A0
  JMP.NE    output
  RET
    
.data
buffer: .db 0, 0, 0, 0
message: .db "Hello world!", 0