# IKP - PubSub servis

Potrebno je implementirati PubSub servis koji može da opslužuje proizvoljan broj klijenata.

Servis treba da implementira sledeći interface:

1. void Connect()
2. void Subscribe(void *topic)
3. void Publish(void *topic, void *type, void *message)

## Dizajn sistema

![IKP - PubSub servis](https://i.postimg.cc/N0HVwskv/IKP.png)

## Formatiranje GitHub repozitorijuma
 
1. Svaki od članova tima je dužan da kreira svoju "granu" na samom repozitorijumu

2. Ime grane sledećeg formata, broj indeksa-godina (PR-158-2018)

3. Prilikom kreiranja COMMIT poruka pratiti sledeću alteraciju Karma konvencije:

        <type>: <subject>
            <body>

        Dozvoljenje <type> vrednosti:
            - feat for a new feature.
            - fix for a bug fix for the user or a fix to a build script.
            - perf for performance improvements.
            - docs for changes to the documentation.
            - style for code formatting changes or UI changes.
            - refactor for refactoring production code.

        <subject> predstavlja kratak opis rešenog problema ili implementacije koja se commit-uje.
            * obavezan deo svakog commit-a

        <body> predstavlja detaljniji opis samo koda ili promena koje su potrebne kako bi se neki problem rešio 
            * možda će biti korisno kasnije, nije obavezno.

        Primer commit-a (bez tela) - perf: added parallelization for taking in requests

4. PULL REQUEST-ove kreiramo posle završavanja svake od celine za koju se dogovorimo da je neko zadužen (ko kako završi dok celine međusobno nisu povezane).

5. Prilikom kreiranja PULL REQUEST-ova svaki član tima je dužan da zatraži odobrenje od svih ostalih članova.

## Formatiranje koda
1. camelCase notacija, kapitalizacija prvog slova druge reči.