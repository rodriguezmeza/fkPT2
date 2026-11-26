# fkPT2 tutor context

The executable starts `scripts/ia_tutor.py` with `options=IA-tutor`.
It searches the current source documentation and recursive README files,
the shared parameter definitions, reference PDFs when a text extractor is
available, and local discussion history. See [the guide](../docs/ia_tutor.rst).

Add reviewed project discussion notes as Markdown under `tutor/notes/`.
These are explicitly historical evidence and receive less retrieval weight
than current documentation. Give the chat title, review date, decisions,
and current documentation that verifies each technical claim.

Personal chat exports and saved conversations belong in
`.fkpt2-tutor/chats/`, which Git ignores. Do not put personal transcripts
under `tutor/notes/`. Import JSON exports or Markdown/text transcripts with
`--import-chat PATH`. The tutor has no account or chat-history credentials;
later conversations need another import. Raw exports are never included in
source distributions.
