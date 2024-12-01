# Algoritmo C-SCAN 
O objetivo deste trabalho é implementar a política de escalonamento C-SCAN e, posteriormente, comparar os resultados obtidos com a política FCFS, analisando o desempenho em diferentes cenários de solicitações de E/S. 

No algoritmo C-SCAN, a cabeça de leitura/gravação se move em uma direção para processar as solicitações e, ao atingir a extremidade do disco, retorna diretamente à extremidade oposta sem atender a requisições durante o trajeto de retorno. Por outro lado, o FCFS (First-Come, First-Served) segue uma abordagem mais simples, atendendo as solicitações na ordem em que chegam, sem levar em consideração a posição da cabeça ou o custo de movimentação no disco. 

# Limitações
O algoritmo apresenta duas limitações. A primeira é que a fila de processos deveria ser despachada para escalonamento apenas quando estivesse cheia. No entanto, a implementação não impede que novos processos entrem na fila enquanto ela ainda está liberando os processos já escalonados. Isso faz com que, mesmo após o despacho inicial, novos processos possam ser adicionados, comprometendo a lógica esperada.  

A segunda limitação é a ausência da funcionalidade que permite ao algoritmo ir até a extremidade final do disco. Atualmente, ele só alcança a extremidade inicial. Essa falha ocorreu devido a dificuldades em identificar corretamente o limite final do disco durante a implementação.


# Execução

### Passo 1: Compilação do Projeto  
1. Navegue até a pasta `/linuxdistro/buildroot/modules/pastadoprojeto`.  
2. Execute o comando:  
   ```bash
   make
   ```

### Passo 2: Execução do Script  
1. Mova o script `run.sh` para dentro do diretório `buildroot`.  
2. Execute o script com privilégios de superusuário:  
   ```bash
   sudo ./run.sh
   ```  
   O script realiza as seguintes ações:  
   - Exporta as variáveis necessárias.  
   - Compila as atualizações (`make`).  
   - Executa o QEMU com a raiz configurada como `/dev/sda`.  

### Parametrização do Algoritmo  
O algoritmo aceita os seguintes parâmetros:  
- **`queue_size`**: Define o tamanho da fila de requisições. Recomenda-se valores entre 5 e 100.  
- **`wait_time`**: Tempo máximo de espera para as requisições, em milissegundos. Recomenda-se valores entre 1 ms e 100 ms.  
- **`debug`**: Booleano que ativa o modo de depuração, exibindo mensagens sobre o comportamento do algoritmo no log do kernel.  

Carregue o módulo utilizando:  
```bash
modprobe cscan-iosched queue_size=<valor> wait_time=<valor> debug=<0|1>
```

### Verificando e Alterando a Política de Escalonamento  
1. Para verificar a política de escalonamento atual:  
   ```bash
   cat /sys/block/sda/queue/scheduler
   ```  
   A política ativa estará indicada entre colchetes (`[]`).  

2. Para alterar a política para `C-SCAN`:  
   ```bash
   echo c-scan > /sys/block/sda/queue/scheduler
   ```

### Execução de Testes  
1. Execute o programa `sector_read` para gerar o PID e o bloco sendo acessado.

2. Para informações detalhadas sobre a política de escalonamento, utilize:  
   ```bash
   cat /sys/block/sda/stat
   ```  
   O comando exibirá os seguintes campos:  

| Campo                          | Total |
|--------------------------------|-------|
| Leituras Concluídas            | 1075  |
| Leituras Mescladas             | 88    |
| Setores Lidos                  | 8184  |
| Tempo Gasto em Leituras (ms)   | 204   |
| Gravações Concluídas           | 40    |
| Gravações Mescladas            | 23    |
| Setores Gravados               | 124   |
| Tempo Gasto em Gravações (ms)  | 136   |
| Operações I/O Ativas           | 6     |
