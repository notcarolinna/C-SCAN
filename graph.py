import matplotlib.pyplot as plt

# Nome do arquivo de entrada
file_name = "outputs.txt"

# Lista para armazenar os valores da segunda coluna
second_column_values = []

# Leitura do arquivo
with open(file_name, 'r') as file:
    for line in file:
        # Separar os valores pela vírgula
        parts = line.strip().split(',')
        if len(parts) == 2:
            try:
                # Adicionar o valor da segunda coluna (depois da vírgula)
                second_column_values.append(int(parts[1].strip()))
            except ValueError:
                # Ignorar linhas mal formatadas
                continue

y_values = range(1, len(second_column_values) + 1)

# Plotar o gráfico (invertendo os eixos)
plt.figure(figsize=(10, 6))
plt.plot(second_column_values, y_values, marker='o', linestyle='-', color='b')
plt.title("C-SCAN")
plt.xlabel("Valor")
plt.ylabel("Índice")
plt.grid(True)
plt.tight_layout()

# Exibir o gráfico
plt.show()
