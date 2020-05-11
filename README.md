# [SOA] Proyecto 2: Diseño de un Web Server simplificado

El propósito de este proyecto es contrastar varias posibilidades de diseño de un servidor en un ambiente distribuido.

## 1. Instrucciones de ejecución

- Ejecutar el comando `make` dentro del directorio del proyecto.
- Ejecutar los siguientes comandos para iniciar los servidores y el cliente:
  - Versión secuencial: `./fifo_server`
  - Versión forked: `./fork_server`
  - Versión threaded: `./thread_server`
  - Versión pre-forked: `./pre_forked_server <cantidadDeProcesos>`
  - Versión pre-threaded: `./pre_threated_server <cantidadDeHilos>`
  - Cliente: `./client <ip> <puerto> <archivo> <N-threads> <N-ciclos>`

Nota: dentro de la carpeta server_files se encuentran algunos archivos de ejemplo para que el cliente los solicite y los servidores sean capaces de enviarlos de vuelta. Un ejemplo de ejecución del cliente es:

```./client 127.0.0.1 51719 wuxing.png 3 4```

Para los servidores pre-forked y pre-threaded, se debe usar cntrl-c para detenerlos elegantemente.

## 2. Puertos de los servidores
- Versión secuencial: 51717
- Versión forked: 51718
- Versión threaded: 51719
- Versión pre-forked: 51720
- Versión pre-threaded: 51721

## Desarrollado por
Kevin Hernández, [Steven Solano](https://github.com/solanors20), [Elisa Argueta](https://github.com/elisa7143), and [Jose Pablo Araya](https://github.com/arayajosepablo)
