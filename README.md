# FileMirroring

### _Program de sincronizare a unui arbore de fisiere local cu un arbore similar de la distanta._

##### Componente: 
```
Server concurent
Client
```
Se considera urmatoarea situatie: pe un calculator server exista un arbore de directoare si fisiere care contin tot timpul ultima versiune a unui sistem software.

Mai multe alte calculatoare doresc sa aiba imagini oglinda ale distributiei (mirrors) pe care sa le mentina cat mai la zi. Pentru aceasta, pe server exista un program server care gestioneaza distributia, iar pe fiecare calculator "mirror" cate un program client.

#### Clientul se conecteaza la server si realizeaza aducerea la zi a fisierelor de pe server. Criteriile de aducere la zi vor fi:
  - data ultimei modificari a fisierelor
  - dimensiunea fisierelor (daca se modifica aceasta, implicit se modifica si data ultimei modificari)
  - stergerea fisierelor locale care nu mai exista pe server
  - aducerea fisierelor noi de la server

#### Pasi de actualizare a arborelui de directoare si fisiere a unui client:
  1. Clientul se conecteaza la server
  2. Clientul cere o cale de la server
  3. Daca acea cale exista pe calculatorul clientului, se verifica data ultimei modificari. Daca datele difere pe cele doua calculatoarea, atunci se va actualiza calea respectiva
  4. Daca acea cale nu exista, clientul va cere continutul fisierului sau va crea directorul
  5. Se revine la pasul 2 atata timp cat serverul mai are cai de transmis
  6. Daca o anumita cale nu a fost primita de la server, atunci aceasta va trebui stearsa de pe calculatorul clientului
  
  
#### Ce trimite serverul:

```c
struct {
    char cale_fiser[];
    long data_ultimei_modificari_a_fisierelor;
    char tip_fisier; // folder/fisier
}
```
#### Ce trimite clientul: char request

```c
request = 0 // da-mi urmatoarea cale
request = 1 // da-mi continutul fisierului pentru ultima cale trimisa
```

```sh
dir/dir1/dir2
dir/fise.txt

dir
  dir1
    dir2
  fise.txt
```
  
