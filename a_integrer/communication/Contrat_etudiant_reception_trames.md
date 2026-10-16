# Contrat étudiant — Réception de commandes en trames

## Objectif

Transformer un flux d’octets série en demandes START, STOP et ACK, sans bloquer la boucle principale et sans commander directement les moteurs.

Le mécanisme de drapeaux et le statechart existent déjà : vous ne devez pas créer une nouvelle file d’événements.

## 1. Format des trames

Une trame contient exactement un des mots ASCII majuscules suivants, suivi d’une fin de ligne :

| Mot reçu | Demande à publier |
| --- | --- |
| `START` | `robot_app_request_start()` |
| `STOP` | `robot_app_request_stop()` |
| `ACK` | `robot_app_request_ack()` |

- Fins de ligne acceptées : LF (`\n`), CR (`\r`) ou CRLF (`\r\n`).
- Aucun espace avant, après ou dans le mot. Les minuscules ne sont pas acceptées.
- Une ligne vide est ignorée. CRLF ne doit pas produire deux demandes.
- Un mot inconnu ne publie aucune demande.
- Toute entrée autre qu’une lettre ASCII de A à Z, hors fin de ligne, invalide la trame courante. Un octet nul n’est pas une fin de trame.

## 2. Réception asynchrone

Les octets d’une même trame peuvent arriver pendant plusieurs passages de boucle. Inversement, plusieurs trames peuvent déjà être disponibles dans le tampon série.

- Conserver une trame partielle entre deux appels.
- Ne rien publier avant la réception de sa fin de ligne.
- Une trame complète, reconnue et valide publie exactement une demande.
- Ne pas attendre l’arrivée du prochain octet : si aucun octet n’est disponible, rendre la main.
- Traiter au maximum 8 octets par appel ; les octets restants attendent les appels suivants.
- Dans cette version, il n’y a pas de timeout : une trame inachevée reste en attente jusqu’à une fin de ligne ou un rejet.

## 3. Mémoire et rejet

- Utiliser un tampon fixe de 8 octets : au plus 7 caractères utiles, plus le zéro terminal nécessaire à une chaîne C.
- Ne pas dépasser les limites du tampon, y compris lors de l’ajout du zéro terminal.
- Aucune allocation dynamique ni objet `String`.
- Si la trame dépasse 7 caractères ou contient un caractère interdit, rejeter toute cette trame jusqu’à la prochaine fin de ligne.
- Après cette fin de ligne, être prêt à recevoir une nouvelle trame.
- Ne pas interpréter la fin d’une trame rejetée comme une nouvelle commande.

## 4. Responsabilités et intégration

Le récepteur interprète le texte et publie les demandes via l’API existante. Il ne doit pas :

- modifier directement l’état du robot ;
- appeler les fonctions de transition ou le statechart ;
- appeler une action moteur ;
- décider lui-même si START ou ACK est autorisé dans l’état courant.

Appeler le récepteur dans le contexte principal, au point de service de la supervision, avant le traitement des événements. Ne pas l’exécuter depuis une ISR.

Il ne doit y avoir qu’un seul lecteur du flux série de commandes : supprimer l’ancien traitement qui transforme tout caractère en START. Ne pas utiliser de lecture bloquante, d’attente active ni d’affichage dans le récepteur.

## 5. Publication et traitement sont distincts

Une demande publiée n’entraîne pas nécessairement une transition : le statechart décide selon l’état et les conditions d’autorisation.

- START ne doit autoriser le départ que depuis l’état prévu, avec une observation valide.
- STOP est prioritaire avant toute nouvelle commande de mouvement.
- ACK après STOP ramène à l’attente ; il ne redémarre pas le robot. Un nouveau START est nécessaire.
- Les drapeaux existants ne constituent pas une file FIFO : des demandes identiques peuvent fusionner.
- Lorsque STOP et des demandes START/ACK sont en attente ensemble, la politique existante privilégie STOP et annule les demandes de reprise concernées. Ne pas remplacer cette politique dans le récepteur.

## 6. Vérifications à réaliser

Avant les essais, compléter les résultats attendus, puis relever le résultat obtenu. Tester d’abord la publication des demandes, puis leur traitement par le statechart.

| Entrée ou scénario | Demande(s) attendue(s) | Résultat obtenu / preuve |
| --- | --- | --- |
| `STA`, puis plus tard `RT` suivi de LF | À compléter | À compléter |
| `STOP` suivi de CRLF | À compléter | À compléter |
| Deux trames valides à la suite | À compléter | À compléter |
| Ligne vide | À compléter | À compléter |
| Mot inconnu | À compléter | À compléter |
| Minuscules, espace ou octet nul dans une trame | À compléter | À compléter |
| Plus de 7 caractères, puis LF, puis une trame valide | À compléter | À compléter |
| Aucun octet disponible | À compléter | À compléter |
| Plus de 8 octets disponibles pendant un appel | À compléter | À compléter |
| STOP pendant FOLLOW, puis ACK | À compléter | À compléter |

## 7. Livrable

- Le code du récepteur et son point d’appel.
- Le tableau de vérification complété, avec une trace des demandes ou des états observés.
- Une courte justification de la conservation des trames partielles, du rejet et de la limite de travail par appel.

## Limites et précautions

Le récepteur doit être servi régulièrement. Son budget de lecture ne protège pas d’un débordement du tampon UART si une autre tâche bloque longtemps. Ce protocole ne garantit ni la livraison des commandes ni un arrêt physique immédiat.

Commencer les essais moteurs désactivés ou roues soulevées, avec une coupure physique accessible. Le canal série ne remplace pas un arrêt d’urgence.
