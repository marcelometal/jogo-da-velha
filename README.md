# Jogo da Velha usando sockets

![Example1][example1]
![Example2][example2]

[example1]: https://metaldot.alucinados.com/images/screenshot-velha-1.png
[example2]: https://metaldot.alucinados.com/images/screenshot-velha.png

Nessa primeira versão por decisão de projeto, a porta de conexão
escolhida foi a `5555` e o host o `localhost`.

## Instalando

### Dependências:

* **Glib2** (No Debian: `aptitude install libglib2.0-dev`)
* **GTK2** (No Debian: `aptitude install libgtk2.0-dev`)

### Compilando o server:
```
gcc -g -Wall -o velha velha.c `pkg-config glib-2.0 --cflags --libs`
```

### Compilando o cliente:
```
gcc -g -Wall -o gtk gtk.c `pkg-config gtk+-2.0 --cflags --libs`
```

## Rodando:

Se já foi compilado o cliente e o servidor, basta:

Rodar o servidor:
```
./velha &
```

Abrir o cliente A:
```
./gtk &
```

clicar em “Start new game”
esperar um outro cliente entrar no jogo

Abrir o cliente B
```
./gtk &
```

escolher o jogo no combobox
clicar em “Join existing game”
começar a jogar clicando na posição escolhida

## Métodos do Protocolo:

* **START** (Inicia um novo jogo)
* **JOIN** (Usado para entrar em um jogo existente)
* **PLAY** (Opção escolhida pelo jogador)
* **LIST** (Lista todos os jogos)
* **QUIT** (Termina a conexão do cliente)
