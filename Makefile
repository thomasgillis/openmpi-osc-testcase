default:
	mpic++ src/main.cpp -o ompi_passive
	mpic++ -DACTIVE src/main.cpp -o ompi_active

clean:
	rm ompi_passive ompi_active
