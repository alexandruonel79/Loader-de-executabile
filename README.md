# Loader-de-executabile

Ideea generala a temei a fost bazata pe cazurile precizate in tema:

	1)mapez pagina unde se afla segm fault ul
	2)daca deja am mapat si problema este in acelasi loc rezulta segm fault
	3)daca adresa nu apartine de exec->segments rezulta segm fault

Initial: mi am creat file descriptorul si mi am alocat vectorul vPagini cu 
dimensiunea tuturor paginilor pe care le am calculat parcurgand fiecare segment 
si adunand memsize ul impartit la lungimea unei pagini.

Primul pas a fost sa vad daca sunt pe segmentul bun, deci am parcurs toate 
segmentele printr un for si verificam daca si_addr se incadreaza in limitele 
segmentului.
	Ulterior am cautat pagina deoarece stiam deja ca si_addr apartine acestui 
segment deci si pagina exista. Am parcurs cu un for mergand din pagina in pagina sa 
vad daca ma incadrez.
	Dupa ce am gasit pagina existau cele 2 cazuri: pagina mapata deja si pagina 
nemapata. Cautam pagina in vectorul vPagini si daca exista-> segm fault, iar daca
nu exista mapez, pun in memorie informatia in functie de incadrare si pun 
permisiunile paginii.
