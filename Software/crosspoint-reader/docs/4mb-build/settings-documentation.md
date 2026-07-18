# CrossPoint Reader — Beállítások dokumentációja

> **Forrás:** `src/CrossPointSettings.h`, `src/SettingsList.h`, `src/CrossPointSettings.cpp`  
> **Mentési hely:** SD kártya `/settings.bin` bináris fájl  
> **Fontos:** A gyorsítótár (`/.crosspoint/`) automatikusan érvénytelenítődik ha megváltozik: betűtípus, betűméret, sortávolság, margó, bekezdésköz vagy orientáció.

---

## Kijelző beállítások

### Alvó képernyő (`sleepScreen`)
**Alapértelmezett:** Sötét

Mi jelenjen meg a kijelzőn alvás közben.

| Érték | Leírás |
|-------|--------|
| Sötét | Teljesen fekete képernyő (legjobb az e-ink élettartamának) |
| Világos | Teljesen fehér képernyő |
| Egyéni | A felhasználó által feltöltött kép az SD kártyáról |
| Borító | Az éppen olvasott könyv borítóképe |
| Üres | Megtartja az utoljára megjelenített tartalmat (nem frissít) |
| Borító + egyéni | Borítókép, ha elérhető; egyébként az egyéni kép |

---

### Borítókép megjelenítési módja (`sleepScreenCoverMode`)
**Alapértelmezett:** Illesztés

Csak akkor aktív, ha az alvó képernyő `Borító` vagy `Borító + egyéni` módban van.

| Érték | Leírás |
|-------|--------|
| Illesztés | A kép arányosan kicsinyítve fér el a kijelzőn (maradhat üres sáv) |
| Vágás | A kép kitölti a teljes kijelzőt, a kilógó részek levágódnak |

---

### Borítókép szűrő (`sleepScreenCoverFilter`)
**Alapértelmezett:** Nincs

Csak borítókép megjelenítési módban aktív.

| Érték | Leírás |
|-------|--------|
| Nincs | Eredeti kép, Floyd-Steinberg dithering az e-ink számára |
| Fekete-fehér kontrasztos | Erős kontrasztú fekete-fehér konverzió |
| Invertált fekete-fehér | Ugyanaz, de invertált (fehér-fekete) |

---

### Akkumulátor százalék elrejtése (`hideBatteryPercentage`)
**Alapértelmezett:** Soha

| Érték | Leírás |
|-------|--------|
| Soha | Az akkumulátor szint mindig látható az állapotsávban |
| Olvasásban | Az olvasó nézetben el van rejtve, a főmenüben látható |
| Mindig | Sehol nem jelenik meg |

---

### Frissítési ciklus (`refreshFrequency`)
**Alapértelmezett:** 15 oldal

Az e-ink kijelzők idővel szellemképeket (ghosting) halmoznak fel — halvány lenyomatát a korábbi oldalaknak. A teljesebb (`HALF_REFRESH`) frissítés megszünteti ezeket, de lassabb.

| Érték | Leírás |
|-------|--------|
| 1 oldal | Minden lapozás után teljes tisztítás (~1 mp) |
| 5 oldal | Minden 5. lapozásnál |
| 10 oldal | Minden 10. lapozásnál |
| 15 oldal | Minden 15. lapozásnál *(alapértelmezett)* |
| 30 oldal | Minden 30. lapozásnál (legtöbb szellemkép, leggyorsabb) |

**Technikai részlet:** A HALF_REFRESH az SSD1677 vezérlő gyors OTP hullámformáját használja a kijelző teljes újra-meghajtásához (~750-1000ms szobahőmérsékleten). A közbülső lapozások FAST_REFRESH módban futnak (~150ms, differenciális).

---

### Felhasználói felület témája (`uiTheme`)
**Alapértelmezett:** Lyra

| Érték | Leírás |
|-------|--------|
| Klasszikus | Az eredeti CrossPoint Reader megjelenés |
| Lyra | Modern, letisztult megjelenés (közepes könyvborító) |
| Lyra — 3 borítóval | Lyra téma, a főoldalon 3 könyvborító látható |

---

### Napfény halványulás kompenzáció (`fadingFix`)
**Alapértelmezett:** Kikapcsolva

Erős napsütésben az e-ink panel néha elveszíti a kontraszt egy részét frissítés után.
Ha be van kapcsolva, a firmware a kijelzővezérlőt kikapcsolja minden frissítés után (`turnOffScreen=true`), ami megakadályozza a feszültség-szivárgást a pixelekben.

**Figyelem:** Kissé lassabb lapozást okoz, mert a következő frissítésnél újra be kell kapcsolni a kijelzőt.

---

### Előtér megvilágítás (`frontlightBrightness`)
**Alapértelmezett:** 0 (kikapcsolva)  
**Tartomány:** 0–100%, lépésköz 10%

A kijelző háttérvilágításának (frontlight) fényereje. A 0 érték teljesen kikapcsolja a frontlight-ot. A százalékos érték PWM kitöltési tényezőre van leképezve.

**Hardveres megjegyzés (4MB build):** A frontlight vezérlő GPIO20-ra van kötve (PWM jel). Ez a pin az eredeti hardware designban USB VBUS észlelés volt — a 4MB build átruházta frontlight vezérlésre.

---

## Olvasó beállítások

### Betűtípus (`fontFamily`)
**Alapértelmezett:** Bookerly

| Érték | Betűméretek | Megjegyzés |
|-------|-------------|------------|
| Bookerly | 12, 14, 16, 18pt | Serif, Amazon Kindle stílusú |
| Noto Sans | 12, 14, 16, 18pt | Sans-serif, jó olvashatóság |
| OpenDyslexic | 8, 10, 12, 14pt | Diszlexia-barát betűtípus |

**Figyelem:** A betűtípus megváltoztatása érvényteleníti a gyorsítótárat — az összes könyv újra renderelődik.

---

### Betűméret (`fontSize`)
**Alapértelmezett:** Közepes

| Érték | Bookerly / Noto Sans | OpenDyslexic |
|-------|---------------------|--------------|
| Kicsi | 12pt | 8pt |
| Közepes | 14pt | 10pt |
| Nagy | 16pt | 12pt |
| Nagyon nagy | 18pt | 14pt |

**Gyorsítótár:** Megváltoztatás érvényteleníti a `.crosspoint/` cache-t.

---

### Sortávolság (`lineSpacing`)
**Alapértelmezett:** Normál

A szöveg sorai közötti távolságot befolyásolja. Az értékek betűtípusonként eltérők:

| Érték | Bookerly | Noto Sans / OpenDyslexic |
|-------|----------|--------------------------|
| Szoros | 0.95× | 0.90× |
| Normál | 1.00× | 0.95× |
| Tág | 1.10× | 1.00× |

**Megjegyzés:** A Noto Sans és OpenDyslexic betűtípusok magasabb alapvonal-aránnyal rendelkeznek, ezért már a "Normál" értéknél is 0.95× szorzót kap.

---

### Margó (`screenMargin`)
**Alapértelmezett:** 5px  
**Tartomány:** 5–40px, lépésköz 5px

A szövegterület és a kijelző szélei közötti távolság pixelben. A négy oldal (felső, jobb, alsó, bal) egységesen változik.

**Gyorsítótár:** Megváltoztatás érvényteleníti a cache-t, mert megváltozik a szövegtördelési terület szélessége.

---

### Bekezdések igazítása (`paragraphAlignment`)
**Alapértelmezett:** Sorkizárt

| Érték | Leírás |
|-------|--------|
| Sorkizárt | Mindkét oldal egyenesen záródik (szóközök bővítésével) |
| Bal | Bal oldal egyenes, jobb oldal fogazott |
| Közép | Mindkét oldal fogazott, szöveg középre rendezve |
| Jobb | Jobb oldal egyenes, bal oldal fogazott |
| Könyv stílus | A könyv saját CSS igazítási beállítását használja |

---

### Beágyazott stílusok (`embeddedStyle`)
**Alapértelmezett:** Bekapcsolva

Ha be van kapcsolva, a firmware használja az EPUB fájlban lévő CSS stíluslapokat (betűvastagság, dőlt, fejléc méretek, stb.). Ha ki van kapcsolva, csak az alapértelmezett firmware stílus érvényesül — egységesebb, de elveszíti a könyv saját formázását.

**Mikor kapcsold ki:** Ha egy könyv hibás CSS-sel rendelkezik és a szöveg rosszul jelenik meg.

---

### Elválasztás (`hyphenationEnabled`)
**Alapértelmezett:** Kikapcsolva

Ha be van kapcsolva, a firmware automatikusan elválasztja a hosszú szavakat a sor végén. Csökkenti a sorkizárásból adódó nagy szóközöket, de számítási igényes — lassabb oldaltördelést eredményezhet.

---

### Orientáció (`orientation`)
**Alapértelmezett:** Portré

| Érték | Felbontás (logikai) | Leírás |
|-------|---------------------|--------|
| Portré | 480×800 | Álló, normál |
| Fekvő (jobbra forgatva) | 800×480 | Fektetett, 180°-os rotáció |
| Fordított portré | 480×800 | Álló, fejjel lefelé |
| Fekvő (balra forgatva) | 800×480 | Fektetett, natív panel orientáció |

**Gyorsítótár:** Megváltoztatás érvényteleníti a cache-t, mert megváltozik a szövegtördelési terület mérete.

---

### Bekezdések közötti extra térköz (`extraParagraphSpacing`)
**Alapértelmezett:** Bekapcsolva

Ha be van kapcsolva, a firmware extra térközt ad a bekezdések közé. Javítja az olvashatóságot, de kevesebb szöveg fér el egy oldalon.

---

### Szöveg élsimítás (`textAntiAliasing`)
**Alapértelmezett:** Bekapcsolva

Ha be van kapcsolva, a szöveg szürkeárnyalatos élsimítással jelenik meg (4 szürkeárnyalat). A renderelési folyamat:
1. Fekete-fehér alap render
2. HALF_REFRESH (ghost clearing)
3. Szürkeárnyalatos LSB pass → RAM-ba másolás
4. Szürkeárnyalatos MSB pass → RAM-ba másolás
5. Egyedi LUT-tal FAST_REFRESH a szürke pixelekhez

Ez az üzemmód ~2-3× lassabb lapozást eredményez, de lényegesen szebb szöveget. Képeket tartalmazó oldalakon mindig aktív, függetlenül ettől a beállítástól.

**RAM megjegyzés:** Az élsimítás egy extra 48KB-os puffert igényel a heap-ről a renderelés idejére.

---

### Képek megjelenítése (`imageRendering`)
**Alapértelmezett:** Megjelenítés

| Érték | Leírás |
|-------|--------|
| Megjelenítés | Képek renderelve és megjelenítve (alapértelmezett) |
| Helyőrző | A kép helyén [kép] felirat jelenik meg |
| Elrejtés | Képek teljesen kihagyva, csak a szöveg jelenik meg |

**Mikor hasznos a Helyőrző/Elrejtés:** Nagy képeket tartalmazó könyveknél, ahol a renderelés lassú vagy sok memóriát fogyaszt.

---

## Vezérlés beállítások

### Oldalsó gombok elrendezése (`sideButtonLayout`)
**Alapértelmezett:** Előző / Következő

Az eszköz bal és jobb oldalsó gombjainak lapozási iránya.

| Érték | Felső gomb | Alsó gomb |
|-------|------------|-----------|
| Előző / Következő | Előző oldal | Következő oldal |
| Következő / Előző | Következő oldal | Előző oldal |

---

### Hosszú nyomás fejezet-ugrás (`longPressChapterSkip`)
**Alapértelmezett:** Bekapcsolva

Ha be van kapcsolva, az oldalsó gombok hosszan tartó nyomása (~1 másodperc) az előző/következő fejezetre ugrik lapozás helyett. Rövid nyomás továbbra is lapoz.

---

### Rövid bekapcsológomb funkció (`shortPwrBtn`)
**Alapértelmezett:** Figyelmen kívül

| Érték | Leírás |
|-------|--------|
| Figyelmen kívül | Rövid nyomás nem csinál semmit (csak hosszú nyomás altat) |
| Altatás | Rövid nyomás azonnal alvó módba kapcsol |
| Lapozás | Rövid nyomás a következő oldalra lép |

**Technikai megjegyzés:** Ha `Altatás` van beállítva, a firmware a bekapcsológomb lenyomást 10ms után érzékeli; egyébként 400ms a küszöb.

---

## Rendszer beállítások

### Automatikus alvás időzítő (`sleepTimeout`)
**Alapértelmezett:** 10 perc

Ennyi tétlen idő után a firmware automatikusan deep sleep módba lép.

| Érték | Idő |
|-------|-----|
| 1 perc | 60 000 ms |
| 5 perc | 300 000 ms |
| 10 perc | 600 000 ms *(alapértelmezett)* |
| 15 perc | 900 000 ms |
| 30 perc | 1 800 000 ms |

**Megjegyzés:** Az időzítő minden gombnyomásra nullázódik. Deep sleep alatt az MCU teljesen leáll, wakeup a bekapcsológombbal lehetséges.

---

### Rejtett fájlok megjelenítése (`showHiddenFiles`)
**Alapértelmezett:** Kikapcsolva

Ha be van kapcsolva, a fájlböngészőben a `.` karakterrel kezdődő fájlok és mappák is megjelennek (pl. `.crosspoint`). Alapértelmezetten rejtve vannak.

---

## Állapotsáv testreszabása *(csak weben)*

Ezek a beállítások csak a webes felületen érhetők el, az eszköz menüjéből nem.

### Fejezet oldalszáma (`statusBarChapterPageCount`)
**Alapértelmezett:** Bekapcsolva  
Az aktuális fejezet oldalszámát mutatja az állapotsávban (pl. `3/12`).

### Könyv haladás százaléka (`statusBarBookProgressPercentage`)
**Alapértelmezett:** Bekapcsolva  
A teljes könyv olvasottságát százalékban mutatja.

### Haladássáv (`statusBarProgressBar`)
**Alapértelmezett:** Elrejtve

| Érték | Leírás |
|-------|--------|
| Könyv | A teljes könyv haladását mutatja sávval |
| Fejezet | Az aktuális fejezet haladását mutatja |
| Elrejtve | Nincs haladássáv |

### Haladássáv vastagsága (`statusBarProgressBarThickness`)
**Alapértelmezett:** Normál — Vékony / Normál / Vastag

### Cím megjelenítése (`statusBarTitle`)
**Alapértelmezett:** Fejezet cím

| Érték | Leírás |
|-------|--------|
| Könyv | A könyv neve jelenik meg |
| Fejezet | Az aktuális fejezet neve |
| Elrejtve | Nincs cím az állapotsávban |

### Akkumulátor az állapotsávban (`statusBarBattery`)
**Alapértelmezett:** Bekapcsolva  
Az akkumulátor töltöttségi szintje megjelenik az állapotsáv jobb sarkában.

---

## KOReader szinkronizáció *(csak weben)*

A KOReader Sync protokollt használva szinkronizálható az olvasási haladás más eszközökkel.

| Beállítás | Leírás |
|-----------|--------|
| Felhasználónév | A KOReader szinkronizációs fiók neve |
| Jelszó | A fiók jelszava |
| Szerver URL | A sync szerver címe |
| Dokumentum azonosítás | `Fájlnév` (gyors) vagy `Bináris` (MD5 hash alapú, pontosabb) |

---

## OPDS böngésző *(csak weben)*

Az OPDS protokollt támogató könyvszerverekről tölthető le tartalom.

| Beállítás | Leírás |
|-----------|--------|
| Szerver URL | Az OPDS katalógus elérhetősége |
| Felhasználónév | Opcionális autentikáció |
| Jelszó | Opcionális autentikáció (titkosítva tárolva) |

---

## Megjegyzések a gyorsítótárhoz

Az alábbi beállítások megváltoztatása érvényteleníti az SD kártyán lévő `.crosspoint/` cache könyvtárat. A könyvek automatikusan újra renderelődnek a következő megnyitáskor.

**Cache-t érvényteleníti:**
- Betűtípus (`fontFamily`)
- Betűméret (`fontSize`)
- Sortávolság (`lineSpacing`)
- Margó (`screenMargin`)
- Bekezdésköz (`extraParagraphSpacing`)
- Orientáció (`orientation`)

**Nem érinti a cache-t:**
- Alvó képernyő beállítások
- Gombok kiosztása
- Alvás időzítő
- Frissítési ciklus
- Állapotsáv megjelenítés
- Élsimítás (renderelési mód, de a cache tartalma azonos)
