<h4 align="center">
  🗃️ Filesystem
</h4>

## Projeto

Ferramenta simples para criar,editar e deletar aquivos JSON no ESP-IDF

## Como usar

Primeiramente adicione a biblioteca em seu projeto ` #include "filesystem.h"`

- Montar Partição
    ```c
    filesystem_init()
    ```

- Escrever
    ```c
    writeFile(char *filepath, char *file_data)
    ```

- Editar
    ```c
   editFile(char *filepath, char *json_key, char *json_key_data)
    ```
- Ler
    ```c
    readFile(char *filepath)
    ```