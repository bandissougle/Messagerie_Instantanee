chat :
	g++ -o chatRevu chatRevu.cpp -lnsl -lpthread -lm
	@echo "Opération réussie"
	@echo "Veuillez saisir ""./chat votre_nom_utilisateur" "pour vous enregistrer"

clean:
	rm -f *.o *~ chat
	


