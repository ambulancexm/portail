# portail

Ce projet perso à durer sur un an.

C'est le projet le plus concret que j'ai fait

J'ai trouvé deux verins à vis pour faire avancer les vantaux.

La partie Hard est constituée d'un Ardunio nano fixé sur un circuit imprimé
fabriqué avec Fritzing 

Les diverses commandes de d'entrée sont

- les potentiometres qui pemettent de connaître la position exacte des vantaux
- la gestions de la pression en cas d'ostacles sur par des résistances shunt de 30 ohm
- la gestion des télecommandes en RF433 qui envoie un signal
- un bouton de poteau pour ouvrir directement sans télécommande
- un bouton d'initialisation pour régler les vantaux et d'enregistrer dans l'EEPROM

les sorties

- la gestion de deux relais qui crée un pont en H pour l'ouverture et la fermeture
- un feux clignotant à plusieurs vitesse en marche normale ou en alerte

le portail est autonome sur une batterie 12 V avec un panneau solaire
