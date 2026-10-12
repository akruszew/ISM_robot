# Séance 2 — helpers fournis aux étudiants (C99)

## Ce qui est fourni

- `robot_app.c` : helpers privés au module avec `static`, états/événements,
  drapeaux, `take_event()`, `transition_to()`, actions entry/exit,
  `is_in_running_state()` et minuterie non bloquante.
- `robot_app.h` : petite interface applicative et types `typedef enum` en C.
- `robot_actions.h` : contrats des opérations à raccorder aux pilotes existants.
- `tests/test_helpers.c` : doubles de test sans matériel et assertions sur les helpers.

Aucune classe, référence C++, lambda, allocation dynamique ou récursion.
Structures déclarées avec `typedef`, initialisation `{0}`, fonctions `(void)`.

## A compléter

1. Raccorder les fonctions de `robot_actions.h` à la base matérielle fournie :
   moteurs/encodeurs gauche et droit, capteur numérique deux voies actif à zéro,
   deux correcteurs indépendants, télémétrie. Le raccord matériel est à préparer
   par l'enseignant ; ce n'est pas un nouveau TP de pilotes.
2. Compléter les cinq TODO de `run_statechart()` : START, perte de ligne,
   retour avant T, ACK normal et ACK après résolution du défaut.
3. Produire les ordres START/STOP/ACK depuis les entrées de l'application.
4. Documenter la géométrie du capteur et les signes de correction.

Le squelette distribué ne passe pas IDLE → FOLLOW : il ne permet pas de départ
avant que le travail soit complété. Les callbacks de test ne pilotent aucun moteur.
**Ce dossier n'est pas un programme complet prêt à téléverser.**

## Contrats importants

- `run_statechart(now_us)` décide des transitions, jamais des calculs de PI.
- `transition_to()` est le seul endroit qui change l'état après construction.
  Il ne teste pas les gardes : le dispatcher doit les valider avant l'appel.
- `is_in_running_state()` reconnaît FOLLOW et LOST. Le nom ne signifie pas
  que le robot se déplace : en LOST, les moteurs sont arrêtés.
- `enter_running()` remet à zéro les deux PI uniquement en entrant dans
  le parent. FOLLOW ↔ LOST conserve le parent et ne réinitialise pas les PI.
- `exit_running()` peut rester vide : `state == FOLLOW` autorise déjà la commande.
  Les arrêts restent dans entry IDLE/LOST/STOPPED/FAULT.
- LOST arme T à entry, l'annule à exit. `after(T)` est évalué sans attente.
  Valeur proposée : 500 ms, à valider ; ce n'est pas une garantie de sûreté.
- `take_event()` prend un événement et le consomme même si aucune transition
  ne l'accepte. Priorités : FAULT, INIT_KO, STOP, INIT_OK, ACK, LINE_LOST,
  LINE_FOUND, START. FAULT/INIT_KO vident les demandes ; STOP annule START et ACK.
- Une observation de ligne récente annule le drapeau de ligne opposé : cette
  politique conserve la dernière observation, pas l'historique. Drapeaux =
  au moins une occurrence, sans ordre ni quantité ; file éventuelle en séance 3.
- Tous les producteurs de drapeaux sont applicatifs. Si une ISR écrit ces
  données, revoir la protection des échanges ; `volatile` seul ne suffit pas.
- `due_speed` reste dans la super-loop. Mesurer → traiter les événements →
  commander seulement en FOLLOW → transmettre. Après erreur de commande,
  traiter FAULT immédiatement. Pas de mise à jour des intégrateurs en LOST.
- L'arrêt précède le diagnostic. COAST ne garantit pas une immobilité immédiate.
  La validation du pont, des modes d'arrêt et la coupure physique restent nécessaires.
- Le capteur donne 0 sur ligne et 1 sur blanc. 11 est une observation valide,
  pas une erreur d'API. 00 ne prouve pas un centrage ou une intersection.
- Soustractions de temps non signées ; aucune durée ne doit dépasser un tour
  du compteur. Les périodes 5/50 ms sont visées, pas garanties. Pas de rattrapage.

## Organisation des fichiers du TP

Conserver `main.ino` court : initialiser et appeler l'application. Les helpers,
le statechart, les dates et les drapeaux restent dans UN module `robot_app`.
Ne pas créer un fichier par helper, ni un framework générique.

La version distribuée `.c` permet une vérification C99 sans Arduino. Dans le
projet Arduino existant où les pilotes sont compilés en `.cpp`, l'enseignant
peut nommer ce même fichier `robot_app.cpp`, sans changer sa syntaxe C, et
raccorder les actions dans le projet. Ne pas mélanger des fichiers `.c` et
`.cpp` en supposant que leurs noms de fonctions ont automatiquement la même
liaison : l'adaptation de compilation appartient au squelette fourni.
Les appels imposés par Arduino restent dans l'adaptation matérielle.

Le `main` du projet doit continuer à appeler `robot_app_run(now_us)` après
un échec d'initialisation : INIT_KO mène FAULT. Ne pas bloquer la boucle,
ni rappeler `robot_app_init()` à chaque passage. L'ACK doit être testé en FAULT
avec `robot_fault_resolved()` et les événements périmés vidés avant retour INIT.

## Test sans robot

Depuis ce dossier :

```sh
gcc -std=c99 -Wall -Wextra -Werror -pedantic tests/test_helpers.c -o test_helpers
./test_helpers
```

Le test inclut le `.c` uniquement pour accéder aux helpers privés : ne pas
copier cette technique dans le projet Arduino et ne pas compiler ce test
avec les fichiers de production du robot.

Les tests vérifient les helpers mécaniques, pas les TODO à compléter, la
commande réelle, la sûreté du matériel ou la précision temporelle sur AVR.
