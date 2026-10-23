# PocketBook tools: direct nginx hosting

The primary site is **https://pocketbook.nimbushub.eu/**. The old CoolReader hostname redirects here with paths preserved. nginx serves the pages
and release files directly from disk. Kubernetes is not involved.

## Layout

- `/var/www/coolreader/releases/<release>/`: website files and `downloads/`.
- `/var/www/coolreader/current`: symlink to the active release.
- `/etc/nginx/sites-available/pocketbook.nimbushub.eu`: the main virtual host.
- `/etc/nginx/sites-available/coolreader.nimbushub.eu`: the legacy-domain redirect.
- `/etc/nginx/snippets/coolreader-static.conf`: shared static/download rules.
- `/etc/letsencrypt/live/pocketbook.nimbushub.eu/`: server-managed TLS material.
- `/var/log/nginx/coolreader.{access,error}.log`: site logs.

Keep TLS keys, Certbot accounts and server logs out of Git and the web root.
The existing virtual hosts can continue to use their own routing.

## Publish content

Prepare a release directory containing `docs/index.html`, `installation.html`, `easy-connect.html`, `styles.css`,
`assets/`, `LICENSE.txt`, `release.json`, `easy-connect-release.json`, `robots.txt`, `sitemap.xml` and a `downloads/` directory containing
the current install ZIP, matching source archive, installation text, build metadata,
verification report and checksums described by `docs/release.json`. Keep only the
current build on this server; remove previous release directories after validation. Do not copy the SDK,
firmware image, workspace, SSH keys or nginx configuration into the web root.

Always preserve both tools' current downloads when updating either tool. The internal
`/var/www/coolreader` directory name is retained for continuity.

The pages and installation guides use permanent download paths:

- `/downloads/coolreader-touch-hd3-latest.zip`
- `/downloads/coolreader-touch-hd3-latest.zip.sha256`
- `/downloads/coolreader-touch-hd3-source-latest.tar.gz`
- `/downloads/easy-connect-pb632-latest.zip`
- `/downloads/easy-connect-pb632-latest.zip.sha256`
- `/downloads/easy-connect-pb632-sources-latest.zip`
- `/downloads/easy-connect-pb632-sources-latest.zip.sha256`
- `/downloads/easy-connect-README-latest.md`

Publish each new build at those names inside the new release directory. Do not
change the HTML download links for each release. GitHub release notes use
`https://github.com/dderugin/pocketbook-coolreader/releases/latest`.
Keep the GitHub assets versioned and copy the same bytes to the stable website
filenames. Regenerate website checksum files using the stable names. Update
`docs/release.json` for CoolReader and `docs/easy-connect-release.json` for Easy Connect with
the current revision, source state and checksums. Website publication does not require a GitHub release.
The Easy Connect install ZIP contains only runtime files; keep its matching source/notices archive separate.

On the server, verify the current downloads from their directory:

```sh
sha256sum -c SHA256SUMS.txt
sha256sum -c coolreader-touch-hd3-latest.zip.sha256
```

Use root-owned directories with mode 755 and files with mode 644. Publish into
a new `/var/www/coolreader/releases/<release>` directory, then switch `current`
atomically. For an update, replace `RELEASE_NAME` with the verified new directory name:

```sh
new_release=/var/www/coolreader/releases/RELEASE_NAME
sudo ln -s "$new_release" /var/www/coolreader/current.next
sudo mv -Tf /var/www/coolreader/current.next /var/www/coolreader/current
```

The HTML uses the primary domain for downloads; keep `docs/release.json` and
those links aligned when changing a release. Source archives must match the binary’s source commit. GitHub and repository
history are separate from this server’s current-build-only download policy.

## First nginx setup

The supplied TLS configuration assumes an nginx installation already using
Certbot's `/etc/letsencrypt/options-ssl-nginx.conf` and `ssl-dhparams.pem`.

1. Install `coolreader-static.conf` in `/etc/nginx/snippets/`.
2. Ensure `server_names_hash_bucket_size 64;` is configured in the `http`
   context. `coolreader-server-names.conf` can be installed in
   `/etc/nginx/conf.d/` if that directive is not already set elsewhere.
3. Install `pocketbook-http-bootstrap.conf` as the new site and enable its
   symlink in `/etc/nginx/sites-enabled/`. Run `sudo nginx -t` before reload.
4. Verify `/.well-known/acme-challenge/` serves files from
   `/var/www/letsencrypt`, and that public DNS and port 80 reach this host.
5. Request a certificate using the existing Certbot account:

```sh
sudo certbot certonly --webroot -w /var/www/letsencrypt \
  --cert-name pocketbook.nimbushub.eu -d pocketbook.nimbushub.eu \
  --non-interactive --deploy-hook 'nginx -t && systemctl reload nginx'
```

6. Replace the bootstrap virtual host with `pocketbook.nimbushub.eu.conf`,
   validate with `sudo nginx -t`, then reload. HTTP now redirects to HTTPS,
   while ACME challenges remain accessible over HTTP.
7. Install the service and timer from this directory in `/etc/systemd/system/`,
   run `sudo systemctl daemon-reload`, then
   `sudo systemctl enable --now pocketbook-cert-renew.timer`.

The timer checks this site's certificate twice daily with a randomized delay.
Its stored Certbot deploy hook validates nginx and reloads the new certificate.

## Validate and roll back

Check trusted HTTPS, HTTP-to-HTTPS redirection, the page and all download hashes.
Verify a Range request returns 206, allowing interrupted downloads to resume.
Use `certbot renew --cert-name pocketbook.nimbushub.eu --dry-run` to test renewal.

From a LAN without NAT loopback, use split DNS or curl's `--resolve` option to
map the public hostname to the nginx server's LAN address. This keeps normal
hostname and certificate validation enabled; do not disable TLS verification.

After validating the newly active site, remove older directories under
`/var/www/coolreader/releases/`; keep only the directory targeted by `current`.
To roll back later, redeploy the desired version from Git/source artifacts.
To disable only this site, remove its `sites-enabled` symlink, run `nginx -t`,
then reload nginx. Keep the server’s certificates for recovery.

Keep the legacy CoolReader certificate renewed while its HTTPS redirect is active.
The `coolreader-cert-renew` timer continues to maintain that certificate.
Install `coolreader.nimbushub.eu.conf` for the legacy redirect after the new main site is verified.
