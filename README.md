 <p align="center">
  <img src="images/banniere.png" width="1000">
  <br>
 </p>
 
 <div align="justify">
Le réseau métier des roboticiens et mécatroniciens du CNRS organise des journées thématiques sur les robots quadrupèdes et humanoïdes afin de favoriser le partage de connaissances et les retours d’expérience autour de ces robots. Cette journée est cofinancée par le réseau 2RM et INRIA.
</div>

 
# TP1 — Arrêt d’urgence sans fil pour robots Unitree Go2/G1

 <p align="center">
  <img src="images/Tp1.png" width="300">
  <br>
 </p>

 
Ce TP est le premier TP des journées thématiques *robotique quadrupède et humanoïde 2026*. Cet arrêt d’urgence se base sur le travail réalisé par le laboratoire Inria Paris, équipe Willow: [https://github.com/inria-paris-robotics-lab/wireless-e-stop/tree/master](https://github.com/inria-paris-robotics-lab/wireless-e-stop/tree/master)

---
## 📑 Sommaire

1. [Introduction](#1-introduction)
2. [Objectif du TP](#2-objectif-du-tp)
3. [Principe de fonctionnement](#3-principe-de-fonctionnement)
4. [Composants](#4-composants)
5. [Montage](#5-montage)
   - [5.1 Transmetteur](#51-transmetteur)
   - [5.2 Récepteur](#52-récepteur)
6. [Test avec une LED](#6-test-avec-une-led)
7. [Test sur le robot](#7-test-sur-le-robot)
8. 
## 1. Objectif du TP

Ce TP a pour objectif de comprendre, assembler et tester un prototype d’arrêt d’urgence sans fil pour robots Unitree, notamment les robots Go2 et G1.

À la fin du TP, les participants devront être capables de :

- comprendre le principe d’un arrêt d’urgence matériel ;
- assembler un module émetteur et un module récepteur ;
- tester le fonctionnement du relais avec une LED ;
- comprendre le comportement du système en cas d’arrêt d’urgence, de perte radio ou de perte d’alimentation ;
- connecter et tester le système sur un robot Unitree, sous supervision.

---

## 2. Principe du système

Sur la carte mère des robots Unitree Go2 et G1, deux broches portent le nom `STOP`.

Lorsque ces deux broches sont reliées électriquement en circuit fermé, le robot coupe l’alimentation des moteurs et passe en état d’arrêt d’urgence, généralement signalé par des LEDs rouges.

Le système proposé repose sur deux groupes de composants :

- un **émetteur** côté opérateur ;
- un **récepteur** côté robot.

Ces deux modules communiquent par radiofréquence.

L’émetteur est composé :

- d’un bouton d’arrêt d’urgence ;
- d’une carte Arduino avec module radio nRF24.

Le récepteur est composé :

- d’une carte Arduino avec module radio nRF24 ;
- d’un relais ;
- d’un bouton reset ;
- d’un connecteur jack relié aux broches `STOP` du robot.

Schéma global :

```text
[insérer l'image du schéma]
```

L’émetteur reste à proximité de l’opérateur et est alimenté en USB. Lorsque l’opérateur appuie sur le bouton d’arrêt d’urgence, l’état d’arrêt d’urgence passe à `TRUE`. Cet état est envoyé régulièrement par radio sur un canal prédéfini.

Le récepteur est accroché au robot. Il est alimenté en USB-C et relié aux deux broches `STOP` de la carte mère du robot via un connecteur jack.

Lorsque le récepteur reçoit un état d’arrêt d’urgence actif, ou lorsqu’il ne reçoit plus de message radio pendant plusieurs cycles, il déclenche le relais. Le relais ferme alors le circuit entre les deux broches `STOP`, ce qui place le robot en arrêt d’urgence.

Le relais est câblé en **normalement fermé**. Ainsi, si le récepteur n’est plus alimenté, le circuit se ferme aussi automatiquement et déclenche l’arrêt d’urgence.

Sur la plupart des robots Unitree, lorsque l’arrêt d’urgence est activé, l’alimentation USB-C du récepteur peut être coupée. Dans le cas contraire, il est nécessaire de maintenir le bouton reset du récepteur pendant 3 secondes pour réarmer le système.

---

## 3. Matériel nécessaire

Tout le matériel est fourni le jour du TP.

### Émetteur

- 1 bouton d’arrêt d’urgence ;
- 1 carte Arduino avec nRF24 intégré ;
- 2 fils de connexion ;
- 1 boîtier émetteur ;
- 2 vis ;
- 1 câble USB.

### Récepteur

- 1 relais ;
- 1 carte Arduino avec nRF24 intégré ;
- 1 bouton reset ;
- 5 fils de connexion ;
- 1 vis ;
- 2 grandes vis ;
- 1 boîtier récepteur ;
- 1 câble USB.

---

## 4. Montage du transmitter

Le transmitter correspond au module émetteur côté opérateur.

Le bouton d’arrêt d’urgence est câblé en **normalement fermé** (`NC`).

```text
STOP button              Carte Arduino
--------------------------------------
C / COM           <->    GND
NC                <->    D2
```

Avec ce montage :

```text
Bouton non appuyé  → circuit fermé  → D2 = LOW  → pas d’alarme
Bouton appuyé      → circuit ouvert → D2 = HIGH → alarme
Fil débranché      → circuit ouvert → D2 = HIGH → alarme
```

Cette logique permet de déclencher l’arrêt d’urgence si le bouton est appuyé, mais aussi si le câble du bouton est débranché.

---

## 5. Montage du receiver

Le receiver correspond au module récepteur côté robot.

### Relais

```text
Relais                  Carte Arduino
-------------------------------------
VCC              <->    5V
IN1              <->    D5
GND              <->    GND
```

### Bouton reset

```text
Reset button            Carte Arduino
-------------------------------------
Pin 1            <->    D2
Pin 2            <->    GND
```

### Connexion au robot

Le relais est relié aux deux broches `STOP` du robot via un connecteur jack.

Le montage utilise le relais en **normalement fermé**. Cela signifie que si le récepteur n’est pas alimenté, le circuit `STOP` est fermé et le robot passe en arrêt d’urgence.

---

## 6. Fonctionnement logique

### Transmitter — `transmitter.ino`

Le transmitter :

- lit l’état du bouton d’arrêt d’urgence ;
- convertit cet état en booléen ;
- envoie ce booléen par radio toutes les 100 ms.

La logique utilisée est la suivante :

```text
Bouton non appuyé  → ALARM FALSE
Bouton appuyé      → ALARM TRUE
```

### Receiver — `receiver.ino`

Le receiver :

- écoute les messages radio envoyés par le transmitter ;
- utilise un algorithme de type **leaky bucket** pour détecter une perte de communication ;
- commande le relais connecté aux broches `STOP` du robot.

Lorsque le receiver est alimenté, il place d’abord le relais en circuit ouvert afin d’autoriser le robot. Ensuite, il surveille en continu l’état reçu par radio.

Le relais ferme le circuit `STOP` dans les cas suivants :

- un message `TRUE` est reçu du transmitter ;
- aucun message n’est reçu pendant une certaine période ;
- l’alimentation du receiver est coupée.

Le relais est en **normalement fermé**, c’est-à-dire que si l’alimentation du receiver est coupée, le relais revient naturellement dans l’état qui déclenche l’arrêt d’urgence.

Après un arrêt d’urgence, si le receiver reste alimenté, le système doit être réarmé manuellement en maintenant le bouton reset pendant 3 secondes.

---

## 7. Test avec LED

Avant de connecter le système au robot, il est nécessaire de tester le montage avec une LED et une pile.

Ce test permet de comprendre le fonctionnement du relais et de vérifier que le montage se comporte comme attendu.

Branchez la sortie du relais en circuit fermé avec la LED, comme indiqué sur la photo suivante :

```text
[insérer photo]
```

Dans ce test :

```text
LED allumée  → circuit fermé  → arrêt d’urgence actif
LED éteinte  → circuit ouvert → robot autorisé
```

Comportement attendu :

```text
Receiver non alimenté              → LED allumée
Receiver alimenté et autorisé      → LED éteinte
Bouton d’arrêt d’urgence appuyé    → LED allumée
Transmitter éteint ou signal perdu → LED allumée
```

Il est aussi possible de s’aider de la LED intégrée au module relais, mais il ne faut pas uniquement se fier à elle. Selon les modules, cette LED peut indiquer l’alimentation du relais ou l’état de commande, sans forcément représenter directement l’état réel du circuit `STOP`.

Le test le plus fiable reste de vérifier la continuité au multimètre.

---

## 8. Test sur robot

Une fois le test avec LED validé, le système peut être testé sur un robot Go2 ou G1.

Le receiver est connecté au robot via le connecteur jack relié aux deux broches `STOP`.

Attention : lorsque l’arrêt d’urgence est déclenché, le robot peut tomber brutalement. Le robot doit donc être maintenu ou attaché avant le test.

Procédure de test :

1. Vérifier le montage avec la LED.
2. Vérifier la continuité du relais au multimètre.
3. Brancher le transmitter.
4. Brancher le receiver.
5. Vérifier que le receiver passe en état autorisé.
6. Connecter le receiver aux broches `STOP` du robot via le jack.
7. Appuyer sur le bouton d’arrêt d’urgence.
8. Vérifier que le robot passe bien en arrêt d’urgence.
9. Réarmer le système avec le bouton reset si nécessaire.

---

## 9. Dépannage rapide

### Le receiver passe directement en arrêt d’urgence

Causes possibles :

- le transmitter n’est pas alimenté ;
- le transmitter et le receiver ne sont pas sur le même canal radio ;
- l’adresse radio est différente entre les deux cartes ;
- le bouton d’arrêt d’urgence est déjà appuyé ;
- le signal radio est perdu ;
- le relais est câblé à l’envers ;
- la logique `RELAY_RUN` / `RELAY_STOP` est inversée dans le code.

### Le transmitter affiche `radio.write=ECHEC`

Causes possibles :

- le receiver n’est pas alimenté ;
- le receiver n’écoute pas sur le bon canal ;
- l’adresse radio ne correspond pas ;
- le mauvais firmware est chargé sur une des cartes ;
- les deux cartes sont trop proches ou trop éloignées ;
- l’alimentation d’une carte est instable.

### Le relais semble inversé

Si le relais ferme le circuit quand il devrait l’ouvrir, ou inversement, il faut vérifier la logique utilisée dans le code receiver :

```cpp
#define RELAY_RUN  HIGH
#define RELAY_STOP LOW
```

ou :

```cpp
#define RELAY_RUN  LOW
#define RELAY_STOP HIGH
```

Le bon réglage est celui qui donne le comportement suivant :

```text
Receiver non alimenté          → circuit fermé  → arrêt d’urgence
Receiver alimenté et autorisé  → circuit ouvert → robot autorisé
Receiver en alarme             → circuit fermé  → arrêt d’urgence
```

### Le bouton reset ne fonctionne pas

Vérifier le câblage suivant :

```text
D2  <-> bouton reset <-> GND
```

Le bouton reset doit être maintenu pendant 3 secondes pour réarmer le système après un arrêt d’urgence.
