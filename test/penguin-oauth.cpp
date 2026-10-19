// SPDX-License-Identifier: GPL-2.0-or-later
#include "plugins/penguin-platforms/oauth-helpers.hpp"

#include <cassert>

int main()
{
	using namespace PenguinOAuth;
	assert(Form({{"a+b", "x+y z&=%"}, {"unicode", QString::fromUtf8("Zürich")}}) ==
	       "a%2Bb=x%2By%20z%26%3D%25&unicode=Z%C3%BCrich");
	assert(Form({}).isEmpty());
	assert(Challenge("dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk") ==
	       "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM");
	const auto first = RandomToken();
	assert(first.size() == 43 && first != RandomToken());
	for (const auto character : first)
		assert(character.isLetterOrNumber() || character == '-' || character == '_');

	const auto accepted = ParseCallback("GET /oauth/callback?state=nonce&code=a%2Bb%2Fc HTTP/1.1\r\n", "nonce");
	assert(accepted.status == CallbackStatus::Accepted && accepted.code == "a+b/c");
	assert(ParseCallback("GET /oauth/callback?state=nonce&error=access_denied HTTP/1.1", "nonce").status ==
	       CallbackStatus::Denied);
	assert(ParseCallback("GET /oauth/callback?state=nonce HTTP/1.1", "nonce").status == CallbackStatus::Denied);
	for (const auto *request : {
		     "GET /oauth/callback?state=wrong&code=x HTTP/1.1",
		     "GET /oauth/callback?code=x HTTP/1.1",
		     "GET /oauth/callback?state=nonce&state=wrong&code=x HTTP/1.1",
		     "GET /oauth/callback?state=nonce&code=x&code=y HTTP/1.1",
		     "POST /oauth/callback?state=nonce&code=x HTTP/1.1",
		     "GET /wrong?state=nonce&code=x HTTP/1.1",
		     "GET https://example.com/oauth/callback?state=nonce&code=x HTTP/1.1",
		     "GET //example.com/oauth/callback?state=nonce&code=x HTTP/1.1",
		     "GET /oauth/callback?state=nonce&code=%ZZ HTTP/1.1",
		     "GET /oauth/callback?state=nonce&code=x#fragment HTTP/1.1",
		     "GET /oauth/callback?state=nonce&code=x INVALID",
		     "malformed",
	     })
		assert(ParseCallback(request, "nonce").status == CallbackStatus::Invalid);
	assert(ParseCallback("GET /oauth/callback?state=&code=x HTTP/1.1", "").status == CallbackStatus::Invalid);
}
